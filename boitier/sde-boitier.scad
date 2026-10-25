// SDE - Boîtier imprimable en 3D, socle lumineux, support mural coulissant et pied de table
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
//   assemblage, interieur, sur_table : vues de contrôle (ne pas imprimer).
//
// Repère : X = largeur (gauche -> droite), Y = hauteur (bas -> haut),
// Z = profondeur (0 = dos du corps, vers l'avant = vers la pièce).
// Le corps s'imprime tel quel, dos sur le plateau.
//
// Cotes : carte relevée sur le STEP de KiCad, capteurs mesurés par Isaac le 2026-10-25.

piece = "assemblage"; // [corps, capot, socle, support, pied, assemblage, interieur, sur_table]

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

/* [Port micro-USB de l'ESP32] */
usb_pos = 42.8;      // centre du connecteur depuis le bord gauche du PCB
usb_retrait = 3.8;   // le connecteur est en retrait du bord haut du PCB
usb_z = 13;          // centre du connecteur au-dessus du PCB (mesuré)
usb_l = 13;          // ouverture pour la fiche (bout moulé micro-USB ~11 x 7)
usb_h = 9;

/* [SDS011 : mesuré (fiche technique : 71 x 70 x 23 avec la buse)] */
sds_x = 69.5;        // largeur de la plaque de base (X)
sds_y = 69.5;        // hauteur (Y)
sds_z = 20;          // épaisseur (Z)
sds_cale = 3;        // plots sous le capteur
sds_jeu = 0.5;       // jeu autour du capteur dans son logement
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
bme_trou_y = 3;      // centre des trous depuis le bord du bas du module (supposé)
bme_vis_d = 2.2;     // avant-trou pour vis M2,5 autotaraudeuse
bme_bas = false;     // true = BME680 en bas de la chambre (plus loin de l'ESP32, fil de 15 cm)
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
pied_leve = 12;      // hauteur du bas de la plaque du pied au-dessus de la table
pied_l = 100;        // largeur de la base
pied_avant = 62;     // la base dépasse devant la plaque
pied_arriere = 32;   // et derrière
pied_ep = 5;

/* [Visserie] */
insert_d = 4.0;      // inserts laiton M3 (trou 4,0 mm, profondeur 6)
insert_prof = 6.5;
vis_d = 3.4;
vis_tete = 6.4;      // M3 tête fraisée

// ------------------------------------------------------------------ Cotes dérivées

bossage = 4.2;       // rayon des bossages des vis du capot
bc = paroi + bossage - 0.8;  // centre légèrement noyé dans la paroi
bos_int = bc + bossage;      // les bossages occupent jusqu'à cette distance des bords extérieurs
cloison = 2;
mur = 1.6;           // mur entre le SDS011 et la chambre

int_l = sds_jeu + sds_x + jeu + mur + chambre;   // largeur intérieure
L = int_l + 2 * paroi;

// Compartiment air : le SDS011 passe entre les bossages du bas et ceux de la cloison
sds_x0 = paroi + sds_jeu;
sds_y0 = bos_int + 0.4;
sds_z0 = dos_ep + sds_cale;
sds_x1 = sds_x0 + sds_x;
sds_y1 = sds_y0 + sds_y;
y_cloison = sds_y1 + sds_jeu + bossage - 1;      // bas de la cloison (bossage de cloison au-dessus du capteur)
bas_h = y_cloison - paroi;

// Compartiment électronique : DevKitC qui dépasse en bas, PCB à 0,5 mm de la paroi du haut
haut_h = kit_bas + 1.5 + pcb_h + 0.5;
H = paroi + bas_h + cloison + haut_h + paroi;
int_p = max(pcb_entretoise + pcb_ep + pcb_composants, sds_cale + sds_z + 1) + 1;  // profondeur intérieure
P = dos_ep + int_p;   // profondeur du corps (sans capot)
capot_ep = 2.4;

y_haut0 = y_cloison + cloison;
y_haut1 = H - paroi;

// Mur et chambre d'air frais
mur_x = sds_x1 + jeu;
ch_x0 = mur_x + mur;
ch_x1 = L - paroi;
buse_y = sds_y1 - buse_haut;
buse_zc = sds_z0 + buse_z;
encoche = buse_d + 0.4;   // encoche du mur pour la buse (ouverte vers l'avant, fermée par la languette du capot)

// BME680 (vis en bas du module, fil en haut)
bme_x0 = ch_x0 + 2;
bme_y0 = bme_bas ? paroi + 12 : y_cloison - 2 - bme_h;
bme_trous = [for (s = [-1, 1]) [bme_x0 + bme_l / 2 + s * bme_trou_dx / 2, bme_y0 + bme_trou_y]];

// PCB, centré
pcb_x0 = (L - pcb_l) / 2;
pcb_y0 = y_haut1 - 0.5 - pcb_h;
pcb_z0 = dos_ep + pcb_entretoise;

// Vis du capot : 4 coins + 2 au niveau de la cloison
vis_capot = [
  [bc, bc], [L - bc, bc], [bc, H - bc], [L - bc, H - bc],
  [bc, y_cloison + cloison / 2], [L - bc, y_cloison + cloison / 2]];

// Vis du socle (depuis l'intérieur du corps, têtes fraisées dans le dos)
sv = socle_retrait + socle_paroi + 6;
vis_socle = [[sv, sv + 8], [L - sv - 8, sv + 8], [L / 2 - 25, H - sv], [L / 2 + 25, H - sv]];

// Passage du câble de la bande LED (dos du corps -> intérieur du socle)
trou_led = [L - 30, H - 14];

// Rainure de la glissière : au centre, ouverte en bas du socle
rainure_long = rail_long + 2;
y_rail0 = socle_retrait + 2;   // bas du rail une fois le boîtier en place

// ------------------------------------------------------------------ Outils

module rrect(l, h, r) { offset(r) offset(-r) square([l, h]); }
module boite(l, h, p, r) { linear_extrude(p) rrect(l, h, r); }
module rrect3(l, h, p) { linear_extrude(p) rrect(l, h, min(l, h) / 2 - 0.01); }

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
      // Cloison entre les deux compartiments
      translate([paroi, y_cloison, 0]) cube([int_l, cloison, P]);
      // Mur entre le SDS011 et la chambre d'air frais
      translate([mur_x, paroi - 0.01, 0]) cube([mur, bas_h + 0.02, P]);
      // Bossages des vis du capot
      for (v = vis_capot) translate([v[0], v[1], 0]) cylinder(r = bossage, h = P);
      // Plots du PCB
      for (t = pcb_trous) translate([pcb_x0 + t[0], pcb_y0 + t[1], 0]) cylinder(d = 6, h = pcb_z0);
      // Plots et butées du SDS011
      for (t = sds_trous) translate([sds_x0 + t[0], sds_y0 + t[1], 0]) cylinder(d = 6.5, h = sds_z0);
      guides_sds();
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
      cube([paroi + sds_jeu + 1.01, sortie_l + 2 * jeu, sortie_h + 2 * jeu]);
    // Buse : encoche en U dans le mur, ouverte vers l'avant
    translate([mur_x - 1, buse_y, buse_zc]) rotate([0, 90, 0]) cylinder(d = encoche, h = mur + 2);
    translate([mur_x - 1, buse_y - encoche / 2, buse_zc]) cube([mur + 2, encoche, P]);
    // Port USB de l'ESP32
    trou_usb();
    // Passages des câbles dans la cloison, sur toute la profondeur (à boucher avec un bout de mousse)
    translate([bos_int + 1.5, y_cloison - 1, dos_ep]) cube([12, cloison + 2, P]);     // SDS011
    translate([ch_x0 + 2, y_cloison - 1, dos_ep]) cube([9, cloison + 2, P]);           // BME680
    // Chambre d'air frais : fentes en bas et sur la droite
    for (i = [0 : 3]) translate([ch_x0 + 3 + i * 4.6, -1, dos_ep + 3]) cube([2, paroi + 2, int_p - 6]);
    for (i = [0 : 5]) fente_laterale(L - paroi, bos_int + 4 + i * (bas_h - 2 * bos_int) / 5, dos_ep + 4, int_p - 8, -1);
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
    // Câble de la bande LED
    translate([trou_led[0], trou_led[1], -1]) cylinder(d = 6, h = dos_ep + 2);
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

module trou_usb() {
  zc = pcb_z0 + pcb_ep + usb_z;
  translate([pcb_x0 + usb_pos, H - paroi - 1, zc]) rotate([-90, 0, 0])
    linear_extrude(paroi + 2) translate([-usb_l / 2, -usb_h / 2]) rrect(usb_l, usb_h, 2);
}

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
    // Logo en creux
    translate([L / 2, y_haut0 + haut_h / 2 - 4, capot_ep - 0.6])
      linear_extrude(1) text("SDE", size = 12, halign = "center", valign = "center",
                             font = "Liberation Sans:style=Bold");
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
      // Bossages des inserts
      for (v = vis_socle) translate([v[0], v[1], -9]) cylinder(d = 8, h = 9);
    }
    // Rainure en queue d'aronde, ouverte vers le mur et vers le bas
    // Partie conique : épouse le rail en bout de course (rail en butée en haut)
    translate([L / 2, y_rail0, -socle_ep - 0.01])
      aronde(rail_base + 2 * jeu, rail_tete + 2 * jeu, rail_haut + 0.5, rail_long, rail_conicite);
    // Entrée droite, ouverte sous le socle
    translate([L / 2, socle_retrait - 2, -socle_ep - 0.01])
      aronde(rail_base + 2 * jeu, rail_tete + 2 * jeu, rail_haut + 0.5, y_rail0 - socle_retrait + 2.01, 0);
    // Inserts pour les vis venant du corps
    for (v = vis_socle) translate([v[0], v[1], -insert_prof]) cylinder(d = insert_d, h = insert_prof + 1);
    // Câble de la bande LED : trou en face de celui du corps, sortie par une encoche
    translate([trou_led[0], trou_led[1], -4]) cylinder(d = 6, h = 5);
    translate([L - socle_retrait - rentre - 4, socle_retrait + 14, -socle_ep / 2 - 2]) cube([10, 5, 4]);
  }
}

// ------------------------------------------------------------------ Support mural

plaque_h = rail_long + 4;
module support() {
  difference() {
    union() {
      translate([-plaque_l / 2, 0, 0]) boite(plaque_l, plaque_h, plaque_ep, 3);
      translate([0, 2, plaque_ep - 0.01]) aronde(rail_base, rail_tete, rail_haut, rail_long, rail_conicite);
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
module pied() {
  // Plaque inclinée et rail (même rail que le support mural)
  rotate([-pied_incl, 0, 0]) {
    translate([-plaque_l / 2, 0, 0]) boite(plaque_l, pied_h, pied_ep, 3);
    translate([0, pied_leve + 2, pied_ep - 0.01]) aronde(rail_base, rail_tete, rail_haut, rail_long, rail_conicite);
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

// ------------------------------------------------------------------ Choix

if (piece == "corps") corps();
else if (piece == "capot") translate([L, 0, capot_ep]) rotate([0, 180, 0]) capot();  // face avant sur le plateau
else if (piece == "socle") translate([0, 0, socle_ep]) socle();  // fond sur le plateau
else if (piece == "support") support();
else if (piece == "pied") rotate([90, 0, 0]) pied();             // base sur le plateau
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
  color("RoyalBlue", 0.8) translate([pcb_x0 + 28.8, pcb_y0 - kit_bas, pcb_z0 + pcb_ep + 8.5]) cube([27.94, 55.1, 1.6]);
  color("Silver") translate([pcb_x0 + usb_pos - 4, pcb_y0 + pcb_h - usb_retrait - 5.5, pcb_z0 + pcb_ep + usb_z - 1.5]) cube([8, 5.5, 3]);
  color("Gold", 0.8) translate([pcb_x0 + 4.02, pcb_y0 + 10.95, pcb_z0 + pcb_ep + 8.5]) cube([19.7, 32.56, 9]);
  // BME680
  color("Purple", 0.8) translate([bme_x0, bme_y0, dos_ep + 5]) cube([bme_l, bme_h, 1.6]);
}
