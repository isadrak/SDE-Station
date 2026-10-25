// SDE - Boîtier mural imprimable en 3D, socle lumineux et support coulissant
//
// Quatre pièces (choisir avec la variable `piece`, ou -D piece="corps" en ligne de commande) :
//   corps    : le boîtier, ouvert à l'avant. Compartiment électronique en haut,
//              compartiment air (SDS011) en bas, cellule BME680 isolée en bas à droite.
//   capot    : la façade avant, vissée sur le corps (6 vis M3 dans des inserts laiton).
//   socle    : la pièce vissée au dos du corps. Elle porte la bande LED, tournée vers
//              le mur (effet de halo), et la glissière en queue d'aronde.
//   support  : la plaque vissée au mur, avec le rail en queue d'aronde. On pose le
//              boîtier au-dessus, on le descend : il s'emboîte et se bloque en bas de
//              course. Pour l'enlever, on le remonte.
//   assemblage : vue de contrôle (ne pas imprimer).
//
// Repère : X = largeur (gauche -> droite), Y = hauteur sur le mur (bas -> haut),
// Z = profondeur (0 = dos du corps, vers l'avant = vers la pièce).
// Le corps s'imprime tel quel, dos sur le plateau.
//
// Toutes les cotes marquées « A MESURER » sont provisoires : corrige-les avec tes
// pièces au pied à coulisse avant d'imprimer.

piece = "assemblage"; // [corps, capot, socle, support, assemblage, interieur]

/* [Impression] */
jeu = 0.25;          // jeu général d'emboîtement (mm)
paroi = 2.0;         // épaisseur des parois
dos_ep = 2.5;        // épaisseur du dos (vis de socle fraisées affleurantes)
rayon = 4;           // rayon des arêtes verticales extérieures
$fn = 48;

/* [Carte PCB (A MESURER)] */
pcb_l = 80;          // largeur du PCB (X)
pcb_h = 60;          // hauteur du PCB (Y)
pcb_ep = 1.6;
pcb_trou_dx = 72;    // entraxe des trous des coins en X
pcb_trou_dy = 52;    // entraxe des trous des coins en Y
pcb_entretoise = 6;  // hauteur des plots sous le PCB
pcb_composants = 22; // hauteur du plus haut composant au-dessus du PCB (ESP32 sur connecteurs femelles)
pcb_marge = 1.0;     // jeu entre le bord du PCB et la paroi côté USB

/* [Port USB de l'ESP32 (A MESURER)] */
usb_cote = "gauche"; // [gauche, droite, haut]
usb_pos = 30;        // position du centre du connecteur le long du bord du PCB (depuis le bas, ou depuis la gauche si « haut »)
usb_z = 11.5;        // hauteur du centre du connecteur au-dessus de la face du PCB
usb_l = 13;          // ouverture pour la fiche (micro-USB moulée ~11 x 7 ; USB-C ~12,5 x 6,5)
usb_h = 9;

/* [SDS011 (fiche technique 71 x 70 x 23 ; buse et sortie A MESURER)] */
sds_x = 71;          // côté parallèle à la face des ouvertures, posé à plat contre le dos
sds_y = 70;
sds_z = 23;
sds_cale = 3;        // plots sous le capteur (l'air circule derrière)
sds_garde = 6;       // distance entre la face des ouvertures et la paroi du bas
// Buse d'entrée : diamètre extérieur 8 mm (tuyau 6/8 mm de la fiche technique)
sds_entree_d = 8.0;
sds_entree_x = 12;   // centre de la buse depuis le bord gauche du capteur
sds_entree_z = 11.5; // centre de la buse depuis la face arrière du capteur
// Sortie du ventilateur (rectangle sur la même face que la buse)
sds_sortie_l = 14;   // largeur (X)
sds_sortie_h = 9;    // hauteur (Z)
sds_sortie_x = 52;   // centre depuis le bord gauche du capteur
sds_sortie_z = 11.5; // centre depuis la face arrière du capteur
// Trous de fixation du capteur, depuis son coin bas-gauche
sds_trous = [[5, 65], [66, 5]];
sds_vis_d = 2.2;     // avant-trou pour vis M2,5 autotaraudeuse

/* [BME680 (CJMCU-680, A MESURER)] */
bme_l = 15;
bme_h = 14;
bme_trou_dx = 10;    // entraxe des 2 trous
bme_vis_d = 1.8;     // avant-trou pour vis M2 autotaraudeuse
cellule = 28;        // cellule isolée (côté intérieur), en bas à droite

/* [Bande LED et socle] */
socle_ep = 18;       // distance dos du boîtier -> plaque murale (profondeur du halo)
socle_retrait = 6;   // le boîtier dépasse du socle de cette valeur (cache la bande)
socle_pente = 15;    // inclinaison des flancs vers le mur (degrés)
socle_paroi = 2.4;
bande_l = 10;        // largeur de la bande LED (WS2812B 60 LED/m : 10 mm)

/* [Glissière] */
rail_long = 50;      // longueur du rail mural
rail_base = 22;      // largeur du rail côté mur
rail_tete = 28;      // largeur du rail côté boîtier (queue d'aronde)
rail_haut = 8;
rail_conicite = 0.8; // le rail et la rainure rétrécissent de cette valeur vers le haut (serrage en bout de course)
plaque_ep = 4;
plaque_l = 62;
vis_mur_d = 4.5;     // vis à bois 4 mm, tête fraisée
vis_mur_tete = 9;

/* [Visserie] */
insert_d = 4.0;      // inserts laiton M3 (trou 4,0 mm, profondeur 6)
insert_prof = 6.5;
vis_d = 3.4;
vis_tete = 6.4;      // M3 tête fraisée

// ------------------------------------------------------------------ Cotes dérivées

bossage = 4.2;       // rayon des bossages des vis du capot
sds_dx = 2 * bossage + 2.5;  // le SDS011 laisse passer les bossages de gauche
int_l = max(pcb_l + 2 * pcb_marge + 6, sds_dx + sds_x + 2 + 1.6 + cellule + 1);  // largeur intérieure
bas_h = sds_garde + sds_y + 6;                                           // compartiment air
haut_h = pcb_h + 13;                                                     // compartiment électronique
cloison = 2;
int_p = max(pcb_entretoise + pcb_ep + pcb_composants, sds_cale + sds_z + 1) + 1;  // profondeur intérieure
L = int_l + 2 * paroi;
H = paroi + bas_h + cloison + haut_h + paroi;
P = dos_ep + int_p;   // profondeur du corps (sans capot)
capot_ep = 2.4;

y_cloison = paroi + bas_h;
y_haut0 = y_cloison + cloison;
y_haut1 = H - paroi;

// Capteur SDS011 posé à plat
sds_x0 = paroi + sds_dx;
sds_y0 = paroi + sds_garde;
sds_z0 = dos_ep + sds_cale;

// Cellule BME680
cel_x0 = L - paroi - cellule;
cel_y1 = paroi + cellule;

// PCB
pcb_x0 = usb_cote == "gauche" ? paroi + pcb_marge
       : usb_cote == "droite" ? L - paroi - pcb_marge - pcb_l
       : (L - pcb_l) / 2;
// Sur les côtés, le PCB passe entre les bossages du haut et ceux de la cloison
pcb_y0 = usb_cote == "haut" ? y_haut1 - pcb_marge - pcb_h : y_haut0 + 4.5;
pcb_z0 = dos_ep + pcb_entretoise;
pcb_cx = pcb_x0 + pcb_l / 2;
pcb_cy = pcb_y0 + pcb_h / 2;

// Vis du capot : 4 coins + 2 au niveau de la cloison
bc = paroi + bossage - 0.8;  // centre légèrement noyé dans la paroi
vis_capot = [
  [bc, bc], [L - bc, bc], [bc, H - bc], [L - bc, H - bc],
  [bc, y_cloison + cloison / 2], [L - bc, y_cloison + cloison / 2]];

// Vis du socle (depuis l'intérieur du corps, têtes fraisées dans le dos)
sv = socle_retrait + socle_paroi + 6;
vis_socle = [[sv, sv + 8], [L - sv - 8, sv + 8], [sv + 8, H - sv], [L - sv, H - sv]];

// Passage du câble de la bande LED (dos du corps -> intérieur du socle)
trou_led = [L - 30, H - 14];

// Rainure de la glissière : au centre, ouverte en bas du socle
rainure_long = rail_long + 2;
y_rail0 = socle_retrait + 2;   // bas du rail une fois le boîtier en place

// ------------------------------------------------------------------ Outils

module rrect(l, h, r) { offset(r) offset(-r) square([l, h]); }
module boite(l, h, p, r) { linear_extrude(p) rrect(l, h, r); }

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
      // Bossages des vis du capot
      for (v = vis_capot) translate([v[0], v[1], 0]) cylinder(r = bossage, h = P);
      // Cellule BME680 : deux murs internes jusqu'au capot
      translate([cel_x0 - 1.6, paroi, 0]) cube([1.6, cellule + 1.6, P]);
      translate([cel_x0 - 1.6, cel_y1, 0]) cube([cellule + 1.6, 1.6, P]);
      // Plots du BME680
      for (s = [-1, 1]) translate([cel_x0 + cellule / 2 + s * bme_trou_dx / 2, paroi + cellule / 2, 0])
        cylinder(d = 5, h = dos_ep + 5);
      // Plots du PCB
      for (sx = [-1, 1], sy = [-1, 1])
        translate([pcb_cx + sx * pcb_trou_dx / 2, pcb_cy + sy * pcb_trou_dy / 2, 0])
          cylinder(d = 7.5, h = pcb_z0);
      // Plots et guides du SDS011
      for (t = sds_trous) translate([sds_x0 + t[0], sds_y0 + t[1], 0]) cylinder(d = 6, h = sds_z0);
      guides_sds();
      // Conduits étanches entre les ouvertures du SDS011 et la paroi du bas
      conduits_sds();
      // Déflecteur extérieur entre l'entrée et la sortie (évite de réaspirer l'air rejeté)
      translate([sds_x0 + (sds_entree_x + sds_sortie_x) / 2 - 1, -8, dos_ep])
        cube([2, 8.01, int_p]);
    }
    // Inserts du capot
    for (v = vis_capot) translate([v[0], v[1], P - insert_prof]) cylinder(d = insert_d, h = insert_prof + 1);
    // Inserts des plots du PCB
    for (sx = [-1, 1], sy = [-1, 1])
      translate([pcb_cx + sx * pcb_trou_dx / 2, pcb_cy + sy * pcb_trou_dy / 2, pcb_z0 - insert_prof])
        cylinder(d = insert_d, h = insert_prof + 1);
    // Avant-trous SDS011 et BME680
    for (t = sds_trous) translate([sds_x0 + t[0], sds_y0 + t[1], 1]) cylinder(d = sds_vis_d, h = 30);
    for (s = [-1, 1]) translate([cel_x0 + cellule / 2 + s * bme_trou_dx / 2, paroi + cellule / 2, 1])
      cylinder(d = bme_vis_d, h = 30);
    // Ouvertures exactes du SDS011 dans la paroi du bas
    ouvertures_sds();
    // Port USB de l'ESP32
    trou_usb();
    // Passage des câbles des capteurs dans la cloison (à l'arrière, côté droit)
    translate([cel_x0 + 2, y_cloison - 1, dos_ep]) cube([14, cloison + 2, 6]);
    // Passage du câble JST du BME680 dans le mur de la cellule
    translate([cel_x0 + 4, cel_y1 - 1, dos_ep]) cube([8, 4, 4]);
    // Aération de la cellule BME680 : fentes en bas et à droite
    for (i = [0 : 4]) translate([cel_x0 + 3 + i * 4.6, -1, dos_ep + 3]) cube([2, paroi + 2, int_p - 6]);
    for (i = [0 : 4]) translate([L - paroi - 1, paroi + 3 + i * 4.6, dos_ep + 3]) cube([paroi + 2, 2, int_p - 6]);
    // Aération du compartiment électronique : entrée en bas, sortie en haut, des deux côtés
    for (c = [0, 1]) {
      x = c == 0 ? 0 : L - paroi;
      for (i = [0 : 2]) {
        fente_laterale(x, y_haut0 + 5 + i * 5, dos_ep + 4, int_p - 8, c == 0 ? 1 : -1);
        fente_laterale(x, y_haut1 - 6 - i * 5, dos_ep + 4, int_p - 8, c == 0 ? 1 : -1);
      }
    }
    // Aération du compartiment air (côté gauche, la droite est occupée par la cellule)
    for (i = [0 : 3]) fente_laterale(0, paroi + 20 + i * 14, dos_ep + 4, int_p - 8, 1);
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
  // Équerres aux 4 coins du capteur (rails de positionnement)
  e = 8; t = 1.6; hz = sds_z0 + 8;
  for (c = [[0, 0, 1, 1], [1, 0, -1, 1], [0, 1, 1, -1], [1, 1, -1, -1]]) {
    cx = sds_x0 - jeu + c[0] * (sds_x + 2 * jeu);
    cy = sds_y0 - jeu + c[1] * (sds_y + 2 * jeu);
    translate([cx, cy, 0]) {
      translate([c[2] > 0 ? -t : 0, c[3] > 0 ? -t : -e, 0]) cube([t, e + t, hz]);
      translate([c[2] > 0 ? -t : -e, c[3] > 0 ? -t : 0, 0]) cube([e + t, t, hz]);
    }
  }
}

module conduits_sds() {
  hy = sds_garde - 0.4;  // s'arrête juste sous la face du capteur
  // Collier de la buse d'entrée
  translate([sds_x0 + sds_entree_x, paroi, sds_z0 + sds_entree_z]) rotate([-90, 0, 0])
    cylinder(d = sds_entree_d + 2 * jeu + 3.2, h = hy);
  // Conduit de la sortie
  translate([sds_x0 + sds_sortie_x - sds_sortie_l / 2 - jeu - 1.6, paroi,
             sds_z0 + sds_sortie_z - sds_sortie_h / 2 - jeu - 1.6])
    cube([sds_sortie_l + 2 * jeu + 3.2, hy, sds_sortie_h + 2 * jeu + 3.2]);
}

module ouvertures_sds() {
  translate([sds_x0 + sds_entree_x, -1, sds_z0 + sds_entree_z]) rotate([-90, 0, 0])
    cylinder(d = sds_entree_d + 2 * jeu, h = paroi + sds_garde + 2);
  translate([sds_x0 + sds_sortie_x - sds_sortie_l / 2 - jeu, -9,
             sds_z0 + sds_sortie_z - sds_sortie_h / 2 - jeu])
    cube([sds_sortie_l + 2 * jeu, paroi + sds_garde + 10, sds_sortie_h + 2 * jeu]);
}

module trou_usb() {
  zc = pcb_z0 + pcb_ep + usb_z;
  if (usb_cote == "gauche" || usb_cote == "droite") {
    x = usb_cote == "gauche" ? -1 : L - paroi - 1;
    translate([x, pcb_y0 + usb_pos, zc]) rotate([0, 90, 0])
      linear_extrude(paroi + 2) translate([-usb_h / 2, -usb_l / 2]) rrect(usb_h, usb_l, 2);
  } else {
    translate([pcb_x0 + usb_pos, H - paroi - 1, zc]) rotate([-90, 0, 0])
      linear_extrude(paroi + 2) translate([-usb_l / 2, -usb_h / 2]) rrect(usb_l, usb_h, 2);
  }
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
    // Fentes devant la cellule BME680
    for (i = [0 : 3]) translate([cel_x0 + 3, paroi + 4 + i * 5.5, -1]) rrect3(cellule - 6, 2.2, capot_ep + 2);
    // Fentes hautes du compartiment électronique (sortie d'air chaud)
    for (i = [0 : 5]) translate([L / 2 - 30 + i * 11, y_haut1 - 8, -1]) rrect3(6, 2.2, capot_ep + 2);
    // Logo en creux
    translate([L / 2, y_haut0 + haut_h / 2 - 4, capot_ep - 0.6])
      linear_extrude(1) text("SDE", size = 12, halign = "center", valign = "center",
                             font = "Liberation Sans:style=Bold");
  }
}
module rrect3(l, h, p) { linear_extrude(p) rrect(l, h, min(l, h) / 2 - 0.01); }

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

// ------------------------------------------------------------------ Choix

if (piece == "corps") corps();
else if (piece == "capot") translate([L, 0, capot_ep]) rotate([0, 180, 0]) capot();  // face avant sur le plateau
else if (piece == "socle") translate([0, 0, socle_ep]) socle();  // fond sur le plateau
else if (piece == "support") support();
else if (piece == "interieur") { color("WhiteSmoke") corps(); fantomes(); }
else {
  color("WhiteSmoke") corps();
  color("Gainsboro") translate([0, 0, P]) capot();
  color("DimGray") socle();
  // Support en position : plaque contre le mur, rail dans la rainure
  color("SteelBlue") translate([L / 2, y_rail0 - 2, -socle_ep - plaque_ep]) support();
  // Bande LED
  color("Gold") translate([0, 0, 0]) bande();
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
  color("DarkSlateGray", 0.8) translate([sds_x0, sds_y0, sds_z0]) cube([sds_x, sds_y, sds_z]);
  color("Black") translate([sds_x0 + sds_entree_x, sds_y0 - 7, sds_z0 + sds_entree_z]) rotate([-90, 0, 0])
    cylinder(d = sds_entree_d, h = 7);
  color("ForestGreen", 0.8) translate([pcb_x0, pcb_y0, pcb_z0]) cube([pcb_l, pcb_h, pcb_ep]);
  color("RoyalBlue", 0.8) translate([pcb_x0 + 4, pcb_cy - 14, pcb_z0 + pcb_ep + 8.5]) cube([54.4, 27.9, 4.5]);
  color("Gold", 0.8) translate([pcb_x0 + 62, pcb_cy - 10, pcb_z0 + pcb_ep + 8.5]) cube([9, 20, 33]);
  color("Purple", 0.8) translate([cel_x0 + (cellule - bme_l) / 2, paroi + (cellule - bme_h) / 2, dos_ep + 5])
    cube([bme_l, bme_h, 1.6]);
}
