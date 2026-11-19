#!/usr/bin/env node
// Mise en service d'une station SDE à l'atelier, avant de la remettre au client.
//
// L'ordinateur se connecte au Wi-Fi de la station (SDE-XXXX, mot de passe
// « sde-station »), puis :
//   node provision.mjs --host <adresse EMQX>.emqxsl.com [--alerts] [--app <url de l'appli>]
//
// Le script :
//   1. lit le nom de la station (SDE-XXXX) sur http://192.168.4.1/api/info ;
//   2. tire un mot de passe MQTT au hasard et l'enregistre dans la station
//      (identifiant = nom de la station) ;
//   3. avec --alerts, active les alertes ntfy sur un sujet tiré au hasard ;
//   4. ajoute la station à sortie/stations.csv et sortie/emqx-utilisateurs.csv ;
//   5. écrit la fiche client à imprimer : sortie/fiche-SDE-XXXX.html (QR codes
//      du Wi-Fi de la station, de l'appli avec l'accès à distance déjà réglé,
//      et de l'abonnement ntfy).
//
// Côté EMQX Cloud (une seule fois pour toutes les stations), dans
// Access Control > Authorization, deux règles « All Users » :
//   allow  pubsub  sde/${username}/#
//   deny   pubsub  #
// Chaque identifiant (station et appli du client) ne voit alors que les
// sujets de sa propre station. Puis, pour chaque station, ajouter l'utilisateur
// dans Access Control > Authentication (ou importer sortie/emqx-utilisateurs.csv).
//
// Le dossier sortie/ contient des mots de passe : il n'est pas versionné.

import { randomBytes } from "node:crypto";
import { appendFileSync, existsSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import QRCode from "qrcode";

const here = dirname(fileURLToPath(import.meta.url));
const out = join(here, "sortie");

function args() {
  const a = process.argv.slice(2), o = { station: "http://192.168.4.1", app: "https://isadrak.github.io/SDE-Station/", alerts: false };
  for (let k = 0; k < a.length; k++) {
    const key = a[k].replace(/^--/, "");
    if (key === "alerts") o.alerts = true;
    else if (key === "dry-run") o.dry = true;
    else o[key] = a[++k];
  }
  if (!o.host) {
    console.error("Usage : node provision.mjs --host <deployment>.emqxsl.com [--alerts] [--station http://192.168.4.1] [--app <url>] [--dry-run]");
    process.exit(2);
  }
  return o;
}

// Mot de passe sans caractères ambigus, ~119 bits
function secret(n = 20) {
  const abc = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
  return Array.from(randomBytes(n), b => abc[b % abc.length]).join("");
}

async function station(base, path, form) {
  const r = await fetch(base + path, form ? { method: "POST", body: new URLSearchParams(form) } : {});
  const j = await r.json().catch(() => ({}));
  if (!r.ok || j.ok === false) throw new Error(`${path} : ${j.msg || j.error || r.status}`);
  return j;
}

const esc = s => String(s).replace(/[&<>"]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" })[c]);
const qr = text => QRCode.toString(text, { type: "svg", margin: 1, errorCorrectionLevel: "M" });

async function main() {
  const o = args();
  const info = o.dry ? { fw: "test", wifi: { ap_ssid: "SDE-TEST" } } : await station(o.station, "/api/info");
  const id = info.wifi?.ap_ssid;
  if (!/^SDE-[0-9A-F]{4}$/.test(id || "") && !o.dry) throw new Error(`Nom de station inattendu : ${id}`);
  console.log(`Station ${id}, firmware ${info.fw}`);

  const pass = secret();
  if (!o.dry) await station(o.station, "/api/cloud", { host: o.host, user: id, pass });
  console.log("Accès à distance enregistré dans la station.");

  let topic = null;
  if (o.alerts) {
    topic = `${id.toLowerCase()}-${randomBytes(8).toString("hex")}`;
    if (!o.dry) await station(o.station, "/api/cmd", { cmd: `alert.topic ${topic}` });
    console.log(`Alertes activées : ntfy.sh/${topic}`);
  }

  mkdirSync(out, { recursive: true });
  const date = new Date().toISOString().slice(0, 10);
  const list = join(out, "stations.csv");
  if (!existsSync(list)) writeFileSync(list, "station,identifiant,mot_de_passe,sujet_ntfy,date\n");
  appendFileSync(list, `${id},${id},${pass},${topic || ""},${date}\n`);
  // Import des utilisateurs du « Built-in Database » d'EMQX
  const users = join(out, "emqx-utilisateurs.csv");
  if (!existsSync(users)) writeFileSync(users, "user_id,password,is_superuser\n");
  appendFileSync(users, `${id},${pass},false\n`);

  // Lien de l'appli : mêmes identifiants que la station, station désignée
  // (l'identifiant ne peut pas s'abonner à sde/+/…).
  const code = Buffer.from(JSON.stringify({ host: o.host, user: id, pass, station: id }), "utf8").toString("base64");
  const appUrl = `${o.app}#acces=${encodeURIComponent(code)}`;
  const wifiQr = `WIFI:T:WPA;S:${id};P:sde-station;;`;
  const ntfyUrl = topic ? `https://ntfy.sh/${topic}` : null;

  const blocks = [
    ["1. Brancher la station", null, "Branchez la station sur un chargeur USB (1 A ou plus). Elle mesure tout de suite ; le CO2 est prêt après 3 minutes."],
    ["2. La relier au Wi-Fi de la maison", await qr(wifiQr), `Scannez ce code (Wi-Fi « ${esc(id)} », mot de passe « sde-station »), ouvrez <b>http://192.168.4.1</b>, puis Réglages › Wi-Fi › Connecter.`],
    ["3. Voir la station de partout", await qr(appUrl), "Scannez ce code sur votre téléphone : l'appli s'ouvre déjà reliée à votre station. Ajoutez-la à l'écran d'accueil (Partager › Sur l'écran d'accueil)."],
  ];
  if (ntfyUrl) blocks.push(["4. Recevoir les alertes", await qr(ntfyUrl), "Installez l'appli gratuite <b>ntfy</b>, puis scannez ce code pour vous abonner : CO2 trop élevé, air trop humide."]);

  const html = `<!doctype html><html lang="fr-CA"><head><meta charset="utf-8"><title>Station ${esc(id)}</title>
<style>
  @page { size: letter; margin: 14mm; }
  body { font: 11pt/1.45 system-ui, sans-serif; color: #111; max-width: 180mm; margin: 0 auto; }
  h1 { font-size: 20pt; margin: 0 0 2mm; } .sub { color: #555; margin: 0 0 8mm; }
  .b { display: grid; grid-template-columns: 38mm 1fr; gap: 6mm; align-items: center; padding: 4mm 0; border-top: 1px solid #ddd; break-inside: avoid; }
  .b svg { width: 38mm; height: 38mm; } h2 { font-size: 13pt; margin: 0 0 1mm; } p { margin: 0; }
  .foot { margin-top: 8mm; font-size: 9pt; color: #555; border-top: 1px solid #ddd; padding-top: 3mm; }
</style></head><body>
<h1>Station de diagnostic environnemental</h1>
<p class="sub">Station <b>${esc(id)}</b> · mise en service le ${date}</p>
${blocks.map(([t, svg, txt]) => `<div class="b"><div>${svg || ""}</div><div><h2>${t}</h2><p>${txt}</p></div></div>`).join("\n")}
<div class="foot">Gardez cette fiche : les codes 3 et 4 donnent accès à vos mesures. Bouton sous la station (trombone, 8 secondes) : remet le Wi-Fi à zéro, la lumière clignote pour confirmer. Calibration du CO2 : Réglages › Avancé, station dehors ou devant une fenêtre grande ouverte.</div>
</body></html>`;
  const fiche = join(out, `fiche-${id}.html`);
  writeFileSync(fiche, html);
  console.log(`Fiche client : ${fiche}`);
  console.log(`À faire dans EMQX : ajouter l'utilisateur ${id} (ou importer ${users}).`);
}

main().catch(e => { console.error("Erreur :", e.message); process.exit(1); });
