# Boîtier SDE

Boîtier imprimable en 3D pour la station, modélisé sous OpenSCAD. Il se fixe au mur sur une glissière, ou se pose sur un pied de table.

![Sur son pied](apercu/vue-table.png)

## Fichiers

| Fichier | Contenu |
|---|---|
| `sde-boitier.scad` | Modèle paramétrique, toutes les cotes en haut du fichier |
| `stl/sde-corps.stl` | Corps, ouvert à l'avant (s'imprime dos sur le plateau) |
| `stl/sde-capot.stl` | Façade avant, déjà retournée |
| `stl/sde-socle.stl` | Pièce vissée au dos : bande LED et rainure de la glissière |
| `stl/sde-support.stl` | Plaque murale avec son rail |
| `stl/sde-pied.stl` | Pied de table, même rail |
| `apercu/` | Rendus (avant, arrière, intérieur, éclatée, support, pied) |

Encombrement : 104 × 148 mm de face, 31,5 mm d'épaisseur avec le capot, 53,5 mm du mur à la façade avec le socle et la plaque.

## Cotes

- Carte : export STEP de KiCad. Au mur, elle est tournée de 180° (micro-USB en haut, MH-Z19B à gauche). Trous Ø 2,2 (M2) à (79,65 ; 50,05), (5 ; 50), (79,75 ; 3,5) et (5 ; 3,6) depuis le coin bas-gauche. Le DevKitC dépasse de 5,9 mm du bas de la carte.
- Mesuré au pied à coulisse : USB à 13 mm au-dessus de la carte et 3,8 mm en retrait du bord, point le plus haut à 19,5 mm. SDS011 69,5 × 69,5 × 20 mm, buse Ø 5 sur la face droite, sortie 22 × 8 mm sur la face gauche. BME680 16 × 18 mm, 2 trous Ø 3 à 11 mm d'entraxe.
- La fiche du SDS011 annonce 71 × 70 × 23 mm (buse et relief compris). Le boîtier laisse 23,6 mm au-dessus des plots, donc les deux valeurs passent. L'encoche de la buse fait 5,4 mm.

## Organisation interne

![Vue éclatée](apercu/vue-eclatee.png)

![Vue intérieure](apercu/vue-interieur.png)

- **En haut, l'électronique.** Carte vissée sur 4 plots de 4 mm. La fiche micro-USB passe par un trou de 13 × 9 mm dans la paroi du haut (câble à embout fin, moins de 11 × 7 mm). Des fentes sur les côtés et dans le capot évacuent la chaleur de l'ESP32.
- **En bas à gauche, le SDS011**, sur 2 plots contre la paroi gauche. Sa sortie de ventilateur tombe en face d'une ouverture de 22 × 8 mm : l'air mesuré ressort à gauche.
- **En bas à droite, une chambre d'air frais**, séparée du SDS011 par un mur. L'air entre par des fentes à droite, en bas et dans le capot. La buse du SDS011 traverse le mur par une encoche en U, refermée par une languette du capot. Le capteur aspire à droite et rejette à gauche, sans réaspirer son propre rejet.
- **Le BME680** est en haut de cette chambre, dans le flux qui va vers la buse, isolé de l'ESP32 par la cloison. Avec un fil plus long (~15 cm), `bme_bas = true` le place en bas de la chambre, à 7 cm de l'ESP32 au lieu de 2.
- La cloison a deux fentes pour les câbles du SDS011 et du BME680, à boucher avec de la mousse pour que la chaleur ne descende pas.

## Fixation murale

![Support](apercu/vue-support.png)

La plaque (62 × 54 mm) se visse au mur avec 2 vis de 4 mm. Elle porte un rail vertical en queue d'aronde de 50 mm ; le socle a la rainure correspondante, ouverte vers le bas. On descend le boîtier sur le rail, on le remonte de 5 cm pour l'enlever. Le rail et la rainure se resserrent de 0,8 mm vers le haut, pour que le serrage n'arrive qu'en fin de course (réglable avec `jeu` et `rail_conicite`).

La station pèse environ 200 g : chevilles à placo ou Molly sur du placoplâtre, chevilles nylon de 6 mm sur béton ou brique.

## Pied de table

![Pied, vue de côté](apercu/vue-table-cote.png)

Le pied porte le même rail que la plaque murale, donc le boîtier passe de l'un à l'autre sans rien démonter. Base de 100 × 94 mm, plaque inclinée de 10° vers l'arrière, bas du boîtier à 5 mm de la table pour ne pas boucher les fentes d'entrée d'air. Inclinaison et hauteur réglables (`pied_incl`, `pied_leve`).

## Bande LED

![Vue arrière](apercu/vue-arriere.png)

Le socle tient le boîtier à 22 mm du mur. Ses flancs sont inclinés de 15° et le boîtier les dépasse de 6 mm : la bande est invisible de face et n'éclaire que le mur autour, dans la couleur de la note.

- Bande WS2812B 5 V, 60 LED/m, IP30 : 22 LED (~37 cm) collées en U sur le socle.
- Câblage : 5 V et GND de la carte, DIN sur IO18 à travers 330 Ω. Sur la carte V1, IO18 n'est pas routé : fil soudé sous le connecteur de l'ESP32, qui passe par le trou de 6 mm du dos.
- Alimentation par l'USB de l'ESP32, courant plafonné à 600 mA par le firmware, chargeur 5 V 2 A conseillé.
- L'ESP32 sort du 3,3 V alors que la bande attend 3,5 V minimum. Ça passe en général sur un fil court ; la V2 du PCB ajoutera un 74AHCT1G125.

Côté firmware, chaque mesure est placée sur une échelle continue de 0 (vert) à 4 (rouge) avec les mêmes normes que l'appli, et la plus mauvaise l'emporte. La teinte change en fondu sur ~8 s, respire en blanc pendant le préchauffage et pulse au-delà de la note 2. Luminosité réglable avec la commande `led <0-100>`. Code : `firmware/src/app/air_color.h` et `firmware/src/hal/led_strip.cpp`, réglages dans `config.h`.

## Impression et montage

PETG ou ABS, buse 0,4, couches de 0,2 mm, 4 périmètres, 20 % de remplissage, aucun support. Toutes les pièces ont été découpées dans PrusaSlicer avant impression :

| Pièce | Temps (profil générique) | PETG |
|---|---|---|
| Corps | 9 h | 82 g |
| Capot | 3 h 20 | 32 g |
| Socle | 4 h 40 | 57 g |
| Support mural | 1 h 10 | 14 g |
| Pied de table | 4 h 15 | 56 g |

Visserie :
- capot et socle : 10 vis M3 × 8 à tête fraisée, directement dans le plastique (trous de 2,5 mm) ; `inserts = true` passe à des inserts laiton ;
- carte : 4 vis M2 × 6 autotaraudeuses ;
- SDS011 : 2 vis M2,5 × 10, BME680 : 2 vis M2,5 × 6 ;
- mur : 2 vis 4 × 30 et chevilles ; table : 4 patins adhésifs.

Ordre de montage : bande sur le socle, socle sur le corps, SDS011 (buse dans l'encoche), BME680, carte, câbles dans la cloison puis mousse, antenne collée contre la paroi droite, capot.

Pour modifier une cote : ouvrir `sde-boitier.scad` dans OpenSCAD, choisir la pièce avec `piece` (`interieur` et `sur_table` affichent l'ensemble monté), F6, puis export STL.

## Version 2 (`sde-boitier-v2.scad`, `stl-v2/`)

![Corps V2](apercu-v2/corps.png)

Même principe que la V1, avec les corrections relevées sur les deux premières stations. À imprimer en PETG. Encombrement : 99,9 × 136 mm de face (V1 : 103,9 × 148,1), 29,1 mm de corps sans le capot. Impression estimée (PrusaSlicer, PETG, 0,2 mm, 20 %, 3 périmètres) : environ 17 h 30 et 184 g pour la version murale (corps 8 h 30, capot 3 h, socle 4 h 05, support 1 h, diffuseur 50 min), pied 4 h 20. Aucune pièce ne demande de supports.

| Changement | Détail |
|---|---|
| Inserts laiton M3 à chaud | Trous de 4,0 mm sur 6,5 mm dans le corps (capot) et le socle (corps) ; bossages élargis à 9,2 mm (`inserts = false` revient aux vis directes). |
| SDS011 tourné | Capteur tourné d'un quart de tour (sens horaire, vu de face) par rapport à sa fiche. Le connecteur et la sortie du ventilateur sont en haut : le câble d'origine se branche droit, sans soudure, et ses fils montent tout droit vers la carte. La buse est en bas : elle entre dans une encoche qui débouche sous le boîtier (fermée à l'avant par une languette du capot) et aspire directement l'air de la pièce, sans conduit. Un peu de mastic ou de mousse autour de la buse rend l'entrée étanche. |
| Galerie de 12 mm | Au-dessus du SDS011 : place pour la fiche du connecteur, quelle que soit sa position sur la face. La cloison est largement ouverte au-dessus du capteur : l'air rejeté par le ventilateur monte, ventile l'électronique et sort par les fentes du haut et par dix fentes du capot, loin de la chambre d'air frais. |
| USB-C en bas | DevKitC couché, prise vers la droite. Une rallonge courte (10 à 20 cm, micro-USB mâle COUDÉE à 90° vers le bas, vers USB-C femelle, qui transmet les données et pas seulement le courant) descend par une fente de 8,5 × 12 mm dans la cloison (à boucher au mastic) puis le long de la paroi droite de la chambre. Son bout femelle moulé, sans oreilles, se glisse par l'avant dans un fourreau en bas de la chambre (section prévue 7 × 13 mm, `usbc_corps`, à ajuster au pied à coulisse) et se tient avec un point de colle chaude ; une lèvre de 0,6 mm le retient et laisse passer la fiche. Le client branche son câble USB-C sans ouvrir le boîtier ; le flash par USB et l'appli par câble passent aussi par là. Sur le pied, la prise tombe à côté de la plaque : le câble descend et part tout droit vers l'arrière, sans contourner le pied. |
| Chambre d'air frais | Fermée en haut par une cloison double (lame d'air de 3 mm) qui la sépare de l'électronique, ouverte sur la pièce par le bas, la paroi droite et le capot. Largeur 24 mm : BME680 à gauche, au-dessus de la prise USB-C (fils d'environ 10 cm, passés par la même fente que la rallonge), rallonge le long de la paroi droite. Avec la correction de température réglable dans l'appli (Réglages › Avancé). |
| Carte V2 en bande | 77 × 34 mm au lieu de 83 × 53 : MH-Z1911A debout à gauche, DevKitC couché à droite. Le compartiment du haut passe de 61 à 35,5 mm. Cotes pour KiCad dans la section suivante. |
| Clip du support | Lame souple taillée dans le flanc droit du rail (support et pied), avec un bossage qui tombe dans un creux de la rainure en bout de course. |
| Trou de la bande LED | Oblong 10 × 6 mm dans le dos et le socle (Ø 6 trop juste en V1). |
| Bouton Wi-Fi | Trou Ø 1,6 dans le capot et guide jusqu'au bouton BOOT (à droite de la prise, vue de face, sur le DevKitC V4) : trombone enfoncé 8 à 20 s puis relâché = Wi-Fi remis à zéro. Position à confirmer avec la carte V2 (`boot_dx`, `boot_dy`). |
| Logo | Trois vagues de l'icône de l'appli gravées sous « SDE ». |
| Diffuseur | Pièce `diffuseur` en PETG naturel : jupe à 4 mm des flancs du socle, devant la bande, serrée par une lèvre côté mur. |

### Carte V2 en bande : cotes pour KiCad

Repère : coin bas-gauche de la carte, vue côté composants, telle qu'elle est posée au mur.

| Élément | Cote |
|---|---|
| Carte | 77 × 34 mm, 1,6 mm, sur plots de 4 mm (broches : 2 mm dessous au plus) |
| Trous de fixation | Ø 2,2 mm (vis M2) en (3,5 ; 3,5), (3,5 ; 30,5) et (52 ; 17). Le troisième est sous le DevKitC, entre ses deux rangées de broches : visser avant de l'enficher. |
| DevKitC V4 | Couché, prise micro-USB vers la droite, centré en hauteur (axe y = 17). Son bout côté prise dépasse de 0,1 mm du bord droit de la carte : il occupe x = 22 à 77,1. Barrettes femelles de 8,5 mm. |
| MH-Z1911A | Debout (19,7 × 32,9 mm), x = 0,5 à 20,2, centré en hauteur. |
| Connecteurs JST (SDS011, BME680, bande LED) | Sous le DevKitC, entre ses rangées de broches (7,5 mm de haut au plus : JST XH droit, 7 mm, convient), à brancher avant d'enficher le DevKitC ; ou coudés sur le bord bas de la carte. Les fils du SDS011 arrivent par le bas à gauche (galerie), ceux du BME680 et la rallonge par le bas à droite. |
| Hauteur des composants | 20 mm au plus au-dessus de la carte. |
| Bouton BOOT | Trou de trombone dans le capot à 4,5 mm du bout du DevKitC et 8,5 mm au-dessus de l'axe de la prise (`boot_dx`, `boot_dy`, à confirmer sur le DevKitC). |

Les positions de la prise USB et du bouton BOOT dépendent de la carte V2 (en cours sous KiCad) : les régler en tête du fichier une fois la carte dessinée, puis réexporter les STL :

```
for p in corps capot socle support pied diffuseur; do
  openscad -o stl-v2/sde-v2-$p.stl -D "piece=\"$p\"" sde-boitier-v2.scad
done
```
