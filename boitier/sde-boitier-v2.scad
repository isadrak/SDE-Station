// SDE V2 - Boîtier imprimable en 3D (PETG), socle lumineux, support mural à clip et pied de table
//
// Changements par rapport à la V1 (sde-boitier.scad) :
//   - inserts laiton M3 à chaud dans le corps et le socle (bossages élargis) ;
//   - SDS011 décalé de 12 mm vers la droite : son connecteur, sur la face
//     gauche, se branche enfin ; un conduit relie sa sortie d'air à la paroi ;
//   - micro-USB en bas : le DevKitC est tourné, prise vers le bas, au-dessus d'un
//     canal vertical entre le SDS011 et la chambre d'air frais. La fiche entre
//     par le dessous du boîtier et remonte le canal jusqu'à la prise ;
//   - cloison double (lame d'air de 3 mm) et passages de câbles ronds : la
//     chaleur de l'ESP32 ne descend plus vers la chambre d'air frais ;
//     BME680 en bas de la chambre par défaut ;
//   - conduit d'entrée du SDS011 (pièce « conduit ») : de la buse jusqu'à une
//     grille sur la paroi droite, joint torique ou mousse contre le mur ;
//     le capteur n'aspire que l'air extérieur ;
//   - languette à clip sur le rail (support et pied) : le boîtier ne glisse plus ;
//   - passage oblong de 10 x 6 mm pour les fils de la bande LED ;
//   - trou pour trombone devant le bouton BOOT (8 s = Wi-Fi remis à zéro) ;
//   - logo de l'appli (trois vagues) gravé sous « SDE » ;
//   - pièce « diffuseur » : jupe translucide emboîtée sur le socle, devant la bande.
//
// Pièces (choisir avec la variable `piece`, ou -D piece="corps" en ligne de commande) :
//   corps    : le boîtier, ouvert à l'avant. Compartiment électronique en haut,
//              compartiment air en bas : SDS011 à gauche, chambre d'air frais à droite
//              (entrée de la buse du SDS011 et BME680).
//   capot    : la façade avant, vissée sur le corps (6 vis M3 dans des inserts laiton).
//   socle    : la pièce vissée au dos du corps. Elle porte la bande LED, tournée vers
//              le mur (effet de halo), et la rainure en queue d'aronde.
//   support  : la plaque vissée au mur, avec le rail en queue d'aronde. On pose le
//              boîtier au-dessus, on le descend : il s'emboîte et se bloque en bas de
//              course. Pour l'enlever, on le remonte.
//   pied     : la même glissière sur un pied à poser sur une table (incliné de 10°).
//   conduit  : tube d'entrée d'air du SDS011, grille extérieure (s'imprime grille en bas).
//   diffuseur: jupe en PETG naturel (translucide) devant la bande LED.
//   assemblage, interieur, sur_table : vues de contrôle (ne pas imprimer).
//
// Repère : X = largeur (gauche -> droite), Y = hauteur (bas -> haut),
// Z = profondeur (0 = dos du corps, vers l'avant = vers la pièce).
// Le corps s'imprime tel quel, dos sur le plateau.
//
// Cotes : carte relevée sur le STEP de KiCad, capteurs mesurés au pied à coulisse.

piece = "assemblage"; // [corps, capot, socle, support, pied, conduit, diffuseur, assemblage, interieur, sur_table]

/* [Impression] */
jeu = 0.25;          // jeu général d'emboîtement (mm)
paroi = 2.0;         // épaisseur des parois
dos_ep = 2.5;        // épaisseur du dos (vis de socle fraisées affleurantes)
rayon = 4;           // rayon des arêtes verticales extérieures
$fn = 48;

/* [Carte PCB : STEP de KiCad, vue côté composants, USB en haut] */
pcb_l = 83;          // largeur du PCB (X)
pcb_h = 53;          // hauteur du PCB (Y)
pcb_ep = 1.6;
// Centres des 4 trous des coins (Ø 2,2 mm, vis M2), depuis le coin bas-gauche du PCB.
// Au mur la carte est tournée de 180° par rapport au STEP : (83 - x, 53 - y).
pcb_trous = [[79.65, 50.05], [5.0, 50.0], [79.75, 3.5], [5.0, 3.6]];
pcb_vis_d = 1.8;     // avant-trou dans les plots pour vis M2 autotaraudeuse
pcb_entretoise = 4;  // hauteur des plots sous le PCB (broches : 2 mm dessous)
pcb_composants = 20; // plus haut point au-dessus du PCB (mesuré : 19,5)
kit_bas = 5.9;       // le DevKitC dépasse du bord bas du PCB (STEP, côté antenne)

/* [Port micro-USB de l'ESP32 : prise vers le bas, au-dessus du canal] */
usb_z = 13;          // centre du connecteur au-dessus du PCB (mesuré en V1, DevKitC sur barrettes femelles)
canal_l = 13;        // largeur intérieure du canal (bout moulé micro-USB ~11 x 7)
canal_paroi = 1.6;
// Bouton BOOT du DevKitC, à côté de la prise (côté droit, prise vers le bas, vu de face)
boot_dx = 8.5;       // depuis le centre de la prise (X) : DevKitC V4, EN à gauche et BOOT à droite de la prise vue de face ; à confirmer
boot_dy = 4.5;       // depuis le bas du DevKitC (Y)
boot_haut = 13.7;    // dessus du bouton au-dessus du PCB (8,5 barrette + 1,6 + ~3,6 bouton)

/* [SDS011 : mesuré (fiche technique : 71 x 70 x 23 avec la buse)] */
sds_x = 69.5;        // largeur de la plaque de base (X)
sds_y = 69.5;        // hauteur (Y)
sds_z = 20;          // épaisseur (Z)
sds_cale = 3;        // plots sous le capteur
sds_jeu = 0.5;       // jeu autour du capteur dans son logement
sds_decal = 12;      // espace à gauche du capteur pour son connecteur (V1 : 0,5 mm, impossible à brancher)
// Buse d'entrée, sur la face droite
buse_d = 5;          // diamètre extérieur
buse_haut = 20;      // centre de la buse depuis le bord du haut
buse_z = 5.5;        // centre de la buse au-dessus de la base
buse_sort = 1.5;     // la buse dépasse de la plaque de base de cette valeur
// Sortie du ventilateur, sur la face gauche
sortie_l = 22;       // longueur (Y)
sortie_h = 8;        // hauteur (Z)
sortie_y = 12;       // bas de l'ouverture depuis le bord du bas du capteur
sortie_z = 12;       // bas de l'ouverture au-dessus de la base
// Trous de fixation (Ø 3), depuis le coin bas-gauche du capteur
sds_trous = [[59.5, 64.5], [4, 46]];
sds_vis_d = 2.2;     // avant-trou pour vis M2,5 autotaraudeuse

/* [BME680 : mesuré] */
bme_l = 16;
bme_h = 18;
bme_trou_dx = 11;    // entraxe des 2 trous (Ø 3)
bme_trou_y = 6;      // centre des trous depuis le bord du bas du module, côté puce (mesuré)
bme_vis_d = 2.2;     // avant-trou pour vis M2,5 autotaraudeuse
bme_bas = true;      // V2 : BME680 en bas de la chambre, loin de l'ESP32 (fil de 15 cm)
chambre = 28;        // largeur intérieure de la chambre d'air frais

/* [Bande LED et socle] */
socle_ep = 18;       // distance dos du boîtier -> plaque murale (profondeur du halo)
socle_retrait = 6;   // le boîtier dépasse du socle de cette valeur (cache la bande)
socle_pente = 15;    // inclinaison des flancs vers le mur (degrés)
socle_paroi = 2.4;
bande_l = 10;        // largeur de la bande LED (WS2812B 60 LED/m : 10 mm)

/* [Glissière] */
rail_long = 50;      // longueur du rail
rail_base = 22;      // largeur du rail côté mur
rail_tete = 28;      // largeur du rail côté boîtier (queue d'aronde)
rail_haut = 8;
rail_conicite = 0.8; // le rail et la rainure rétrécissent de cette valeur vers le haut (serrage en bout de course)
plaque_ep = 4;
plaque_l = 62;
vis_mur_d = 4.5;     // vis 4 mm, tête fraisée
vis_mur_tete = 9;

/* [Pied de table] */
pied_incl = 10;      // inclinaison vers l'arrière (degrés)
pied_leve = 27;      // hauteur du bas de la plaque du pied au-dessus de la table (V2 : le câble USB sort en bas, 20 mm pour le courber)
pied_l = 100;        // largeur de la base
pied_avant = 62;     // la base dépasse devant la plaque
pied_arriere = 32;   // et derrière
pied_ep = 5;

/* [Conduit du SDS011] */
conduit_de = 8;      // diamètre extérieur du tube
conduit_di = 5.6;    // diamètre intérieur (buse Ø 5)
joint_ep = 1.5;      // joint (mousse ou joint torique 8 x 1,5) entre le tube et le mur
grille_d = 16;       // collerette extérieure avec la grille

/* [Diffuseur] */
diff_ep = 0.8;       // 2 couches de PETG naturel
diff_ecart = 4;      // distance au flanc du socle (bande LED + LED)

/* [Visserie] */
inserts = true;      // true = inserts laiton M3 à chaud (trou 4,0 mm) ; false = vis M3 directement dans le plastique
insert_d = inserts ? 4.0 : 2.5;   // diamètre du trou des vis M3 du capot et du socle
insert_prof = 6.5;   // profondeur du trou (vis M3 x 8)
vis_d = 3.4;
vis_tete = 6.4;      // M3 tête fraisée

// ------------------------------------------------------------------ Cotes dérivées

bossage = inserts ? 4.6 : 4.2;  // rayon des bossages des vis du capot (2,6 mm de matière autour d'un insert)
bc = paroi + bossage - 0.8;  // centre légèrement noyé dans la paroi
bos_int = bc + bossage;      // les bossages occupent jusqu'à cette distance des bords extérieurs
cloison_mur = 1.6;
cloison_air = 3;     // lame d'air entre les deux murs de la cloison
cloison = 2 * cloison_mur + cloison_air;
mur = 1.6;           // mur entre le SDS011 et la chambre

int_l = sds_decal + sds_jeu + sds_x + jeu + mur + chambre + canal_paroi + canal_l;   // largeur intérieure
L = int_l + 2 * paroi;

// Compartiment air : le SDS011 passe entre les bossages du bas et ceux de la cloison
sds_x0 = paroi + sds_decal + sds_jeu;
sds_y0 = bos_int + 0.4;
sds_z0 = dos_ep + sds_cale;
sds_x1 = sds_x0 + sds_x;
sds_y1 = sds_y0 + sds_y;
y_cloison = sds_y1 + sds_jeu + bossage - 1;      // bas de la cloison (bossage de cloison au-dessus du capteur)
bas_h = y_cloison - paroi;

// Compartiment électronique : DevKitC qui dépasse en bas (prise USB vers le bas), PCB à 0,5 mm de la paroi du haut
haut_h = kit_bas + 1.5 + pcb_h + 0.5;
H = paroi + bas_h + cloison + haut_h + paroi;
int_p = max(pcb_entretoise + pcb_ep + pcb_composants, sds_cale + sds_z + 1) + 1;  // profondeur intérieure
P = dos_ep + int_p;   // profondeur du corps (sans capot)
capot_ep = 2.4;

y_haut0 = y_cloison + cloison;
y_haut1 = H - paroi;

// Mur du SDS011, canal de la fiche USB (ouvert vers l'avant, fermé par le capot)
// et chambre d'air frais
mur_x = sds_x1 + jeu;
canal_x0 = mur_x + mur;
canal_x1 = canal_x0 + canal_l;
ch_x0 = canal_x1 + canal_paroi;
ch_x1 = L - paroi;
usb_x = (canal_x0 + canal_x1) / 2;          // centre de la prise du DevKitC (X)
usb_zc = dos_ep + pcb_entretoise + pcb_ep + usb_z;
canal_z0 = usb_zc - 4.6;                    // fond du canal (la fiche fait ~7 mm d'épaisseur)
canal_fond = 1.0;                           // plancher du canal, imprimé en pont au-dessus du conduit
buse_y = sds_y1 - buse_haut;
buse_zc = sds_z0 + buse_z;
encoche = buse_d + 0.4;   // encoche du mur pour la buse (ouverte vers l'avant, fermée par la languette du capot)

// BME680 (vis en bas du module, fil en haut)
bme_x0 = ch_x0 + 2;
bme_y0 = bme_bas ? paroi + 12 : y_cloison - 2 - bme_h;
bme_trous = [for (s = [-1, 1]) [bme_x0 + bme_l / 2 + s * bme_trou_dx / 2, bme_y0 + bme_trou_y]];

// PCB, centré
pcb_x0 = (L - pcb_l) / 2;   // la carte V2 place la prise du DevKitC à usb_x (voir LISEZMOI)
pcb_y0 = y_haut1 - 0.5 - pcb_h;
pcb_z0 = dos_ep + pcb_entretoise;

// Vis du capot : 4 coins + 2 au niveau de la cloison
vis_capot = [
  [bc, bc], [L - bc, bc], [bc, H - bc], [L - bc, H - bc],
  [bc, y_cloison + cloison / 2], [L - bc, y_cloison + cloison / 2]];

// Vis du socle (depuis l'intérieur du corps, têtes fraisées dans le dos)
sv = socle_retrait + socle_paroi + 6;
vis_socle = [[sv, sv + 8], [L - sv - 8, sv + 8], [L / 2 - 25, H - sv], [L / 2 + 25, H - sv]];

// Passage des fils de la bande LED (dos du corps -> intérieur du socle), au centre, loin des vis du socle
trou_led = [L / 2, H - 14];
led_oblong = [10, 6];   // V1 : Ø 6, trop juste pour les 3 fils et leur gaine

// Prise USB et bouton BOOT (DevKitC tourné, prise vers le bas, juste au-dessus de la cloison)
kit_y0 = y_cloison + cloison + 1.5;         // bas du DevKitC
boot = [usb_x + boot_dx, kit_y0 + boot_dy];

// Rainure de la glissière : au centre, ouverte en bas du socle
rainure_long = rail_long + 2;
y_rail0 = socle_retrait + 2;   // bas du rail une fois le boîtier en place

// ------------------------------------------------------------------ Outils

module rrect(l, h, r) { offset(r) offset(-r) square([l, h]); }
module boite(l, h, p, r) { linear_extrude(p) rrect(l, h, r); }
module rrect3(l, h, p) { linear_extrude(p) rrect(l, h, min(l, h) / 2 - 0.01); }
module oblong(l, h, p) { linear_extrude(p) translate([-l / 2, -h / 2]) rrect(l, h, min(l, h) / 2 - 0.01); }

// Logo de l'appli : trois vagues (sinusoïdes de trait e), largeur l
module vagues(l = 26, a = 1.6, e = 1.4, pas = 4.2) {
  n = 24;
  for (k = [0 : 2]) translate([0, (1 - k) * pas])
    for (i = [0 : n - 1]) hull() for (j = [i, i + 1])
      translate([-l / 2 + l * j / n, a * sin(360 * 1.5 * j / n)]) circle(d = e, $fn = 12);
}

// Fente de ventilation à travers une paroi latérale, inclinée vers le bas à l'extérieur
module fente_laterale(x, y, z0, long, cote) {
  translate([x, y, z0])
    multmatrix([[1, 0, 0, 0], [cote * 0.6, 1, 0, 0], [0, 0, 1, 0]])
      translate([-1, -1, 0]) cube([paroi + 2, 2, long]);
}

// Profil de queue d'aronde (section dans le plan X-Z), base en z=0
module profil_aronde(base, tete, haut) {
  polygon([[-base / 2, 0], [base / 2, 0], [tete / 2, haut], [-tete / 2, haut]]);
}

// Rail conique : section pleine en bas (y=0), réduite de `con` en largeur en haut (y=long)
module aronde(base, tete, haut, long, con) {
  hull() {
    rotate([-90, 0, 0]) linear_extrude(0.01) mirror([0, 1, 0]) profil_aronde(base, tete, haut);
    translate([0, long, 0]) rotate([-90, 0, 0]) linear_extrude(0.01) mirror([0, 1, 0])
      profil_aronde(base - con, tete - con, haut);
  }
}

// ------------------------------------------------------------------ Corps

module corps() {
  difference() {
    union() {
      difference() {
        boite(L, H, P, rayon);
        translate([paroi, paroi, dos_ep]) boite(int_l, H - 2 * paroi, P, 1.5);
      }
      // Cloison double entre les deux compartiments (lame d'air au milieu)
      difference() {
        translate([paroi, y_cloison, 0]) cube([int_l, cloison, P]);
        translate([paroi + 3, y_cloison + cloison_mur, dos_ep]) cube([int_l - 6, cloison_air, P]);
      }
      // Canal de la fiche USB : mur côté chambre et plancher (le mur du SDS011 fait l'autre côté)
      translate([canal_x1, paroi - 0.01, 0]) cube([canal_paroi, bas_h + 0.02, P]);
      translate([canal_x0 - 0.01, paroi - 0.01, canal_z0 - canal_fond]) cube([canal_l + 0.02, bas_h + 0.02, canal_fond]);
      // Mur entre le SDS011 et la chambre d'air frais
      translate([mur_x, paroi - 0.01, 0]) cube([mur, bas_h + 0.02, P]);
      // Bossages des vis du capot
      for (v = vis_capot) translate([v[0], v[1], 0]) cylinder(r = bossage, h = P);
      // Plots du PCB
      for (t = pcb_trous) translate([pcb_x0 + t[0], pcb_y0 + t[1], 0]) cylinder(d = 6, h = pcb_z0);
      // Plots et butées du SDS011
      for (t = sds_trous) translate([sds_x0 + t[0], sds_y0 + t[1], 0]) cylinder(d = 6.5, h = sds_z0);
      guides_sds();
      // Conduit de sortie : de la sortie du ventilateur à la paroi gauche (l'air
      // rejeté ne revient pas dans le boîtier), il tient aussi le capteur à gauche
      translate([paroi - 0.01, sds_y0 + sortie_y - jeu - 1.2, 0])
        cube([sds_x0 - sds_jeu - paroi + 0.01, sortie_l + 2 * jeu + 2.4, sds_z0 + sortie_z + sortie_h + jeu + 1.2]);
      // Butée gauche en haut du capteur (le connecteur est plus bas, entre les deux)
      translate([paroi - 0.01, sds_y1 - 12, 0]) cube([sds_x0 - sds_jeu - paroi + 0.01, 10, sds_z0 + 8]);
      // Plots du BME680
      for (t = bme_trous) translate([t[0], t[1], 0]) cylinder(d = 6, h = dos_ep + 5);
    }
    // Inserts du capot
    for (v = vis_capot) translate([v[0], v[1], P - insert_prof]) cylinder(d = insert_d, h = insert_prof + 1);
    // Avant-trous PCB, SDS011 et BME680
    for (t = pcb_trous) translate([pcb_x0 + t[0], pcb_y0 + t[1], 1]) cylinder(d = pcb_vis_d, h = pcb_z0);
    for (t = sds_trous) translate([sds_x0 + t[0], sds_y0 + t[1], 1]) cylinder(d = sds_vis_d, h = 30);
    for (t = bme_trous) translate([t[0], t[1], 1]) cylinder(d = bme_vis_d, h = 30);
    // Sortie du ventilateur : trou exact dans la paroi gauche
    translate([-1, sds_y0 + sortie_y - jeu, sds_z0 + sortie_z - jeu])
      cube([sds_x0 + 1.01, sortie_l + 2 * jeu, sortie_h + 2 * jeu]);
    // Buse : encoche en U dans le mur, ouverte vers l'avant
    translate([mur_x - 1, buse_y, buse_zc]) rotate([0, 90, 0]) cylinder(d = encoche, h = mur + 2);
    translate([mur_x - 1, buse_y - encoche / 2, buse_zc]) cube([mur + 2, encoche, P]);
    // Canal USB : ouvert en bas du boîtier et dans la cloison, jusqu'à la prise
    translate([canal_x0, -1, canal_z0]) cube([canal_l, y_cloison + cloison + 2, P]);
    // Passages ronds des câbles dans la cloison (connecteur JST compris), à boucher au mastic
    for (x = [bos_int + 8, ch_x0 + 7]) translate([x, y_cloison - 1, dos_ep + 5]) rotate([-90, 0, 0]) cylinder(d = 7, h = cloison + 2);
    // Chambre d'air frais : fentes en bas et sur la droite (sauf devant le conduit)
    for (i = [0 : 4]) translate([ch_x0 + 3 + i * 4.6, -1, dos_ep + 3]) cube([2, paroi + 2, int_p - 6]);
    for (i = [0 : 5]) let(y = bos_int + 4 + i * (bas_h - 2 * bos_int) / 5)
      if (abs(y - buse_y) > grille_d / 2 + 2) fente_laterale(L - paroi, y, dos_ep + 4, int_p - 8, -1);
    // Conduit du SDS011 : du mur du capteur à la paroi droite, sous le plancher du canal
    translate([canal_x0 - 0.01, buse_y, buse_zc]) rotate([0, 90, 0]) cylinder(d = conduit_de + 0.4, h = L);
    // Aération du compartiment électronique : entrée en bas, sortie en haut, des deux côtés
    for (c = [0, 1]) {
      x = c == 0 ? 0 : L - paroi;
      for (i = [0 : 2]) {
        fente_laterale(x, y_haut0 + bossage + 4 + i * 5, dos_ep + 4, int_p - 8, c == 0 ? 1 : -1);
        fente_laterale(x, y_haut1 - bos_int - i * 5, dos_ep + 4, int_p - 8, c == 0 ? 1 : -1);
      }
    }
    // Vis du socle, têtes fraisées à l'intérieur
    for (v = vis_socle) translate([v[0], v[1], -1]) {
      cylinder(d = vis_d, h = dos_ep + 2);
      translate([0, 0, dos_ep + 1 - 1.8]) cylinder(d1 = vis_d, d2 = vis_tete, h = 1.81);
    }
    // Fils de la bande LED
    translate([trou_led[0], trou_led[1], -1]) oblong(led_oblong[0], led_oblong[1], dos_ep + 2);
  }
}

module guides_sds() {
  // Butées en haut et en bas du capteur (la paroi gauche et le mur le tiennent sur les côtés)
  e = 10; t = 1.6; hz = sds_z0 + 8;
  for (x = [sds_x0 + 6, sds_x1 - 6 - e]) {
    translate([x, sds_y0 - sds_jeu - t, 0]) cube([e, t, hz]);
    translate([x, sds_y1 + sds_jeu, 0]) cube([e, t, hz]);
  }
}

// Le canal doit laisser passer le conduit du SDS011 dessous : vérification
assert(buse_zc + conduit_de / 2 + 0.2 < canal_z0 - canal_fond, "le conduit du SDS011 touche le plancher du canal USB");

// ------------------------------------------------------------------ Capot

module capot() {
  difference() {
    boite(L, H, capot_ep, rayon);
    for (v = vis_capot) translate([v[0], v[1], -1]) {
      cylinder(d = vis_d, h = capot_ep + 2);
      // Tête fraisée côté extérieur (z = capot_ep une fois posé)
      translate([0, 0, capot_ep + 1 - 1.8]) cylinder(d1 = vis_d, d2 = vis_tete, h = 1.81);
    }
    // Fentes devant la chambre d'air frais
    for (i = [0 : 10]) translate([ch_x0 + 3, bos_int + 2 + i * 6.5, -1]) rrect3(chambre - 6, 2.2, capot_ep + 2);
    // Fentes hautes du compartiment électronique (sortie d'air chaud)
    for (i = [0 : 5]) translate([L / 2 - 30 + i * 11, y_haut1 - 8, -1]) rrect3(6, 2.2, capot_ep + 2);
    // Logo en creux : « SDE » et les trois vagues de l'appli dessous
    translate([L / 2, y_haut0 + haut_h / 2 + 2, capot_ep - 0.6])
      linear_extrude(1) text("SDE", size = 12, halign = "center", valign = "center",
                             font = "Liberation Sans:style=Bold");
    translate([L / 2, y_haut0 + haut_h / 2 - 13, capot_ep - 0.6]) linear_extrude(1) vagues();
    // Trou du trombone devant le bouton BOOT
    translate([boot[0], boot[1], -P]) cylinder(d = 1.6, h = P + capot_ep + 1);
  }
  // Guide du trombone jusqu'à 1,5 mm du bouton
  guide = P - (dos_ep + pcb_entretoise + pcb_ep + boot_haut) - 1.5;
  translate([boot[0], boot[1], -guide]) difference() {
    cylinder(d = 4.4, h = guide + 0.01);
    translate([0, 0, -1]) cylinder(d = 1.6, h = guide + 2);
  }
  // Languette qui ferme l'encoche de la buse et appuie le capteur
  difference() {
    translate([mur_x + 0.2, buse_y - encoche / 2 + 0.25, buse_zc - P]) cube([mur - 0.4, encoche - 0.5, P - buse_zc + 0.01]);
    translate([mur_x - 1, buse_y, buse_zc - P]) rotate([0, 90, 0]) cylinder(d = buse_d + 0.3, h = mur + 2);
  }
}

// ------------------------------------------------------------------ Socle lumineux

sl0 = L - 2 * socle_retrait;              // contour côté boîtier
sh0 = H - 2 * socle_retrait;
rentre = socle_ep * tan(socle_pente);     // retrait du contour côté mur
module contour_socle(ext, plus = 0) {
  r2 = rentre * (1 + plus / socle_ep);
  hull() {
    translate([socle_retrait - ext, socle_retrait - ext, -0.01])
      boite(sl0 + 2 * ext, sh0 + 2 * ext, 0.01, 6 + ext);
    translate([socle_retrait + r2 - ext, socle_retrait + r2 - ext, -socle_ep - plus])
      boite(sl0 - 2 * r2 + 2 * ext, sh0 - 2 * r2 + 2 * ext, 0.01, 6 + ext);
  }
}

module socle() {
  difference() {
    union() {
      difference() {
        contour_socle(0);
        // Creux intérieur (fond de 2,5 mm côté boîtier)
        intersection() {
          contour_socle(-socle_paroi, 1);
          translate([0, 0, -socle_ep - 2]) cube([L, H, socle_ep + 2 - 2.5]);
        }
      }
      // Colonne centrale qui porte la rainure
      intersection() {
        translate([L / 2 - (rail_tete + 10) / 2, 0, -socle_ep]) cube([rail_tete + 10, socle_retrait + rainure_long + 8, socle_ep]);
        contour_socle(0);
      }
      // Bossages des vis du corps
      for (v = vis_socle) translate([v[0], v[1], -9]) cylinder(d = inserts ? 9 : 8, h = 9);
    }
    // Rainure en queue d'aronde, ouverte vers le mur et vers le bas
    // Partie conique : épouse le rail en bout de course (rail en butée en haut)
    translate([L / 2, y_rail0, -socle_ep - 0.01])
      aronde(rail_base + 2 * jeu, rail_tete + 2 * jeu, rail_haut + 0.5, rail_long, rail_conicite);
    // Entrée droite, ouverte sous le socle
    translate([L / 2, socle_retrait - 2, -socle_ep - 0.01])
      aronde(rail_base + 2 * jeu, rail_tete + 2 * jeu, rail_haut + 0.5, y_rail0 - socle_retrait + 2.01, 0);
    // Creux du clip, en face du bossage quand le boîtier est en bout de course
    translate([L / 2 + flanc(rail_haut / 2) + jeu + clip_bosse - 1.5, y_rail0 + clip_y, -socle_ep + rail_haut / 2])
      sphere(d = 3.4, $fn = 24);
    // Inserts pour les vis venant du corps
    for (v = vis_socle) translate([v[0], v[1], -insert_prof]) cylinder(d = insert_d, h = insert_prof + 1);
    // Câble de la bande LED : trou en face de celui du corps, sortie par une encoche
    translate([trou_led[0], trou_led[1], -4]) oblong(led_oblong[0], led_oblong[1], 5);
    translate([L - socle_retrait - rentre - 4, socle_retrait + 14, -socle_ep / 2 - 2]) cube([10, 5, 4]);
  }
}

// Clip de la glissière : une lame souple taillée dans le flanc droit du rail,
// libre en haut, porte un bossage qui tombe dans un creux de la rainure en bout
// de course (clic). Pour décrocher, remonter le boîtier d'un coup sec.
clip_long = 22;      // longueur de la lame
clip_ep = 1.8;       // épaisseur de la lame
clip_fente = 1.2;
clip_bosse = 0.7;    // dépassement du bossage
clip_y = rail_long - 4;   // position du bossage depuis le bas du rail
// Abscisse du flanc droit à la hauteur z (rail local, base en z=0), près du haut du rail
function flanc(z) = ((rail_base - rail_conicite) + ((rail_tete - rail_base) * z / rail_haut)) / 2;

module rail() {
  difference() {
    aronde(rail_base, rail_tete, rail_haut, rail_long, rail_conicite);
    // Fente parallèle au flanc, ouverte en haut du rail
    translate([0, rail_long - clip_long, 0]) rotate([-90, 0, 0]) linear_extrude(clip_long + 1) mirror([0, 1, 0])
      polygon([[flanc(0) - clip_ep - clip_fente, -0.01], [flanc(0) - clip_ep, -0.01],
               [flanc(rail_haut) - clip_ep, rail_haut + 0.01], [flanc(rail_haut) - clip_ep - clip_fente, rail_haut + 0.01]]);
  }
  // Bossage
  intersection() {
    translate([flanc(rail_haut / 2) + clip_bosse - 1.5, clip_y, rail_haut / 2]) sphere(d = 3, $fn = 24);
    translate([flanc(rail_haut / 2) - 2, clip_y - 2, 0.6]) cube([4, 4, rail_haut - 1.2]);
  }
}

// ------------------------------------------------------------------ Support mural

plaque_h = rail_long + 4;
module support() {
  difference() {
    union() {
      translate([-plaque_l / 2, 0, 0]) boite(plaque_l, plaque_h, plaque_ep, 3);
      translate([0, 2, plaque_ep - 0.01]) rail();
    }
    // Vis murales fraisées
    for (s = [-1, 1]) translate([s * (plaque_l / 2 - 9), plaque_h / 2, -1]) {
      cylinder(d = vis_mur_d, h = plaque_ep + 2);
      translate([0, 0, plaque_ep + 1 - (vis_mur_tete - vis_mur_d) / 2])
        cylinder(d1 = vis_mur_d, d2 = vis_mur_tete, h = (vis_mur_tete - vis_mur_d) / 2 + 0.01);
    }
    // Repères de centrage (crayon / niveau)
    for (y = [0, plaque_h]) translate([0, y, -1]) cylinder(d = 3, h = plaque_ep + 2, $fn = 3);
  }
}

// ------------------------------------------------------------------ Pied de table
// Repère de la table : Y vers le haut, Z vers l'avant. Le dos de la plaque passe par l'origine.
// S'imprime base sur le plateau (la pièce « pied » est exportée dans ce sens).

pied_h = pied_leve + plaque_h;
module pied() intersection() {
  translate([-500, 0, -500]) cube(1000);   // rien sous la table
  pied_brut();
}
module pied_brut() {
  // Plaque inclinée et rail (même rail que le support mural)
  rotate([-pied_incl, 0, 0]) {
    translate([-plaque_l / 2, 0, 0]) boite(plaque_l, pied_h, pied_ep, 3);
    translate([0, pied_leve + 2, pied_ep - 0.01]) rail();
    // Biseau à 45° sous le rail : il s'imprime sans support et entre dans la rainure
    hull() {
      translate([0, pied_leve + 2, pied_ep - 0.01]) aronde(rail_base, rail_tete, rail_haut, 0.01, 0);
      translate([-rail_base / 2, pied_leve + 2 - rail_haut, pied_ep - 0.01]) cube([rail_base, 0.01, 0.01]);
    }
  }
  // Base
  translate([0, pied_ep, 0]) rotate([90, 0, 0]) linear_extrude(pied_ep)
    translate([-pied_l / 2, -pied_arriere]) rrect(pied_l, pied_arriere + pied_avant, 8);
  // Deux équerres à l'arrière
  for (x = [-plaque_l / 2 + 2, plaque_l / 2 - 5]) hull() {
    translate([x, 0, -pied_arriere + 4]) cube([3, pied_ep, 1]);
    rotate([-pied_incl, 0, 0]) translate([x, 0, -0.5]) cube([3, pied_h - 6, 1]);
  }
}

// ------------------------------------------------------------------ Conduit du SDS011
// Tube de la paroi droite jusqu'au mur du SDS011 : son bout intérieur appuie sur
// un joint (mousse adhésive ou joint torique) autour de la buse. S'emboîte par
// l'extérieur, la collerette et sa grille restent sur la paroi. Pièce posée
// grille en bas sur le plateau.
// Collerette à l'extérieur de la paroi droite ; le joint (dans un lamage de
// 0,8 mm au bout du tube) est écrasé de 0,3 mm contre le mur.
conduit_long = L + 1.6 - canal_x0 - (joint_ep - 0.8 - 0.3);
module conduit() {
  difference() {
    union() {
      cylinder(d = conduit_de, h = conduit_long);
      cylinder(d = grille_d, h = 1.6);
    }
    translate([0, 0, 1.6]) cylinder(d = conduit_di, h = conduit_long);
    // Grille : fentes de 1 mm dans la collerette
    for (i = [-2 : 2]) translate([-grille_d / 2, i * 2.2 - 0.5, -1]) cube([grille_d, 1, 1.6 + 1.01]);
    // Lamage du joint
    translate([0, 0, conduit_long - 0.8]) difference() { cylinder(d = conduit_de + 1, h = 1); cylinder(d = conduit_di + 1, h = 1); }
  }
}

// ------------------------------------------------------------------ Diffuseur
// Jupe translucide qui entoure le socle à diff_ecart de ses flancs, devant la
// bande. Une lèvre côté mur serre sur le socle (qui s'élargit vers le boîtier) :
// on l'enfile par le côté mur et on pousse. S'imprime lèvre en bas.
module diffuseur() {
  difference() {
    union() {
      difference() { contour_socle(diff_ecart + diff_ep); contour_socle(diff_ecart, 1); }
      // Lèvre
      intersection() {
        difference() { contour_socle(diff_ecart + diff_ep); contour_socle(0.15, 1); }
        translate([-50, -50, -socle_ep - 1]) cube([L + 100, H + 100, 1 + 1.6]);
      }
    }
    // Côté boîtier : la jupe s'arrête 1 mm avant le dos
    translate([-50, -50, -1]) cube([L + 100, H + 100, 10]);
    // Passage du rail (bas) et des fils (encoche du socle)
    translate([L / 2 - (rail_tete + 12) / 2, -50, -socle_ep - 2]) cube([rail_tete + 12, socle_retrait + 50 + 6, socle_ep + 4]);
  }
}

// ------------------------------------------------------------------ Choix

if (piece == "corps") corps();
else if (piece == "capot") translate([L, 0, capot_ep]) rotate([0, 180, 0]) capot();  // face avant sur le plateau
else if (piece == "socle") translate([0, H, 0]) rotate([180, 0, 0]) socle();  // côté boîtier (fond plein) sur le plateau
else if (piece == "support") support();
else if (piece == "pied") rotate([90, 0, 0]) pied();             // base sur le plateau
else if (piece == "conduit") conduit();
else if (piece == "diffuseur") translate([0, 0, socle_ep]) diffuseur();   // lèvre sur le plateau
else if (piece == "interieur") { color("WhiteSmoke") corps(); fantomes(); }
else if (piece == "sur_table") rotate([90, 0, 0]) {   // table horizontale dans la vue
  color("SteelBlue") pied();
  // Boîtier en place sur le rail du pied
  rotate([-pied_incl, 0, 0]) translate([-L / 2, pied_leve - (y_rail0 - 2), pied_ep + socle_ep]) boitier_complet();
}
else boitier_complet(true);

module boitier_complet(avec_support = false) {
  color("WhiteSmoke") corps();
  color("Gainsboro") translate([0, 0, P]) capot();
  color("DimGray") socle();
  // Support en position : plaque contre le mur, rail dans la rainure
  if (avec_support) color("SteelBlue") translate([L / 2, y_rail0 - 2, -socle_ep - plaque_ep]) support();
  color("Gold") bande();
  color("White", 0.6) diffuseur();
  color("DimGray") translate([L + 1.6, buse_y, buse_zc]) rotate([0, -90, 0]) conduit();
}

module bande() {
  zc = -socle_ep / 2 - bande_l / 2 + 1;
  r = rentre / 2;
  translate([0, 0, zc]) linear_extrude(bande_l - 2) difference() {
    translate([socle_retrait + r - 0.6, socle_retrait + 10]) rrect(sl0 - 2 * r + 1.2, sh0 - 10 - r + 0.6, 6);
    translate([socle_retrait + r, socle_retrait + 9]) rrect(sl0 - 2 * r, sh0 - 9 - r, 6);
  }
}

// Pièces réelles en transparence, pour vérifier l'encombrement (vue « interieur »)
module fantomes() {
  // SDS011, sa buse et sa sortie
  color("DarkSlateGray", 0.8) translate([sds_x0, sds_y0, sds_z0]) cube([sds_x, sds_y, sds_z]);
  color("Black") translate([sds_x1 - 8.5, buse_y, buse_zc]) rotate([0, 90, 0]) cylinder(d = buse_d, h = 8.5 + buse_sort);
  color("Red", 0.8) translate([sds_x0 - 0.3, sds_y0 + sortie_y, sds_z0 + sortie_z]) cube([0.3, sortie_l, sortie_h]);
  // PCB, DevKitC et MH-Z19B (positions du STEP tournées de 180°), sur connecteurs femelles de 8,5 mm
  color("ForestGreen", 0.8) translate([pcb_x0, pcb_y0, pcb_z0]) cube([pcb_l, pcb_h, pcb_ep]);
  // DevKitC prise vers le bas, au-dessus du canal (carte V2 à dessiner en conséquence)
  color("RoyalBlue", 0.8) translate([usb_x - 27.94 / 2, kit_y0, pcb_z0 + pcb_ep + 8.5]) cube([27.94, 55.1, 1.6]);
  color("Silver") translate([usb_x - 4, kit_y0 - 0.5, usb_zc - 1.5]) cube([8, 5.5, 3]);
  color("Red") translate([boot[0], boot[1], pcb_z0 + pcb_ep + boot_haut - 2]) cylinder(d = 3, h = 2);
  // Fiche micro-USB dans le canal
  color("Black", 0.6) translate([usb_x - 5.5, kit_y0 - 0.5 - 20, usb_zc - 3.5]) cube([11, 20, 7]);
  // BME680
  color("Purple", 0.8) translate([bme_x0, bme_y0, dos_ep + 5]) cube([bme_l, bme_h, 1.6]);
}

// Accès pour les vérifications d'assemblage (fichier utilisé avec `use`)
function P_() = P;
module conduit_place() translate([L + 1.6, buse_y, buse_zc]) rotate([0, -90, 0]) conduit();
module support_place() translate([L / 2, y_rail0 - 2, -socle_ep - plaque_ep]) support();
