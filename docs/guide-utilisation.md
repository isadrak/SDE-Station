# Guide d'utilisation de la station SDE

Ce guide accompagne le [README](../README.md). Les fichiers binaires (firmware `-merged.bin` et `-ota.bin`, appli Android `sde.apk`) se téléchargent dans les [Releases](../../../releases) du dépôt.

## Contenu

| Dossier | Rôle |
|---|---|
| `firmware/` | Firmware de l'ESP32-DevKitC-32E (MH-Z19B, SDS011, BME680, Wi-Fi) |
| `sde-firmware-X.Y.Z-merged.bin` (Releases) | Firmware complet, pour le flash par USB à l'adresse `0x0` |
| `sde-firmware-X.Y.Z-ota.bin` (Releases) | Le même firmware, pour la mise à jour par Wi-Fi |
| `boitier/` | Boîtier imprimable en 3D, socle avec bande LED, support mural coulissant et pied de table (voir `boitier/LISEZMOI.md`) |
| `app/` | Application d'analyse (`sde-analyse.html`), son manifeste, son service worker et ses icônes. Tout est intégré dans le firmware : la station sert elle-même l'application |
| `android/` | Appli Android : `sde.apk` (aussi dans les Releases) et sa source (coquille qui charge la page de la station, section 3 bis) |

## 1. Flasher l'ESP32 en USB (une fois, puis par Wi-Fi)

Inutile si la station a déjà la 0.4.0 ou plus : passe alors par le Wi-Fi (section 4).

1. Branche l'ESP32 en USB.
2. Dans Chrome ou Edge, ouvre https://espressif.github.io/esptool-js/
3. Baudrate 921600, clique sur **Connect** et choisis le port de l'ESP32 (CP210x ou « USB to UART »).
4. Flash Address `0x0`, choisis `sde-firmware-0.9.8-merged.bin`, puis clique sur **Program**.
   Jamais le fichier `-ota.bin` par câble : à l'adresse `0x0`, il écrase le démarreur et la station affiche en boucle « invalid header » (il faut alors refaire cette étape avec le `-merged.bin`).
5. À la fin, clique sur **Disconnect**, ferme l'onglet et appuie sur le bouton **EN** de l'ESP32.

Si Windows ne voit pas l'ESP32, installe le pilote « CP210x USB to UART » de Silicon Labs.

## 2. Utiliser la station en Wi-Fi

Branche la station sur n'importe quel chargeur USB 5 V (1 A ou plus, à cause du ventilateur du SDS011).

**Connexion directe (partout, sans box) :**
1. Sur ton téléphone ou ton ordinateur, connecte-toi au Wi-Fi **SDE-XXXX** (les 4 derniers caractères changent d'une carte à l'autre). Mot de passe par défaut : **sde-station**.
2. Ouvre **http://192.168.4.1** dans le navigateur. Sur beaucoup de téléphones, la page s'ouvre toute seule.

**Sur le Wi-Fi de la maison (optionnel) :**
1. Connecte-toi d'abord en direct. À la première connexion, l'assistant de configuration demande le Wi-Fi tout seul ; sinon, touche **Connecter** sur la carte de l'accueil ou dans **Réglages**, section **Wi-Fi**.
2. Choisis ton réseau dans la liste (la station ne voit que le 2,4 GHz), entre le mot de passe : l'appli attend que la station soit vraiment connectée et dit si le mot de passe est refusé ou si le réseau est introuvable (firmware 0.8.6+).
3. Tu peux ensuite remettre le téléphone sur le Wi-Fi de la maison : l'appli (même installée depuis `192.168.4.1`) essaie aussi la dernière adresse connue de la station sur ton réseau. La station reste joignable sur son propre Wi-Fi et devient aussi accessible depuis ton réseau à l'adresse **http://sde.local** ou à l'IP affichée dans la carte Wi-Fi. Elle s'en souvient après un redémarrage.

Pense à changer le mot de passe du point d'accès dans la même carte. Ce mot de passe protège aussi les mises à jour et la remise à zéro de l'historique.

**En USB (toujours possible) :** ouvre `app/sde-analyse.html` dans Chrome ou Edge, puis ouvre « Autre adresse, câble USB ou fichier » puis **Câble USB**.

Le lien **Voir une démonstration** montre l'application avec des données simulées, sans matériel.

**Si l'appli ne trouve pas la station :** elle réessaie toute seule (après 1, 2, 4, 8 puis toutes les 15 s) et l'indique sous le bouton **Rechercher la station**, inutile de rafraîchir. Si ça dure :
- sur un téléphone Android connecté au Wi-Fi SDE-XXXX, coupe les **données mobiles** (ou réponds « Rester connecté » quand Android dit que le réseau n'a pas Internet) : sinon Android envoie les requêtes par le réseau mobile et la station est introuvable ;
- `http://sde.local` dépend du mDNS, qui marche mal sur certains appareils : utilise plutôt l'IP de la station affichée dans **Réglages**, section **Wi-Fi**.

## 3. Installer l'application sur le téléphone (Android, Chrome)

Chrome n'installe une vraie application que sur un site en HTTPS. La station ne sert que du HTTP, alors il faut l'autoriser une fois :

1. Connecte le téléphone au Wi-Fi **SDE-XXXX**.
2. Dans Chrome, ouvre `chrome://flags/#unsafely-treat-insecure-origin-as-secure`
3. Dans la case, écris `http://192.168.4.1,http://sde.local` (ajoute l'IP de la station sur ton réseau si tu l'utilises, toujours séparée par une virgule), puis choisis **Enabled** et touche **Relaunch**. Chaque adresse est une application distincte pour Chrome.
4. Ouvre `http://192.168.4.1` (ou `http://sde.local`). Quelques secondes après la connexion, une barre **Installer l'appli** apparaît en bas de l'écran (**Plus tard** la cache 7 jours). Si Chrome ne propose pas l'installation, le bouton explique quoi régler.

Si tu as fermé la barre, le bouton **Installer** reste dans **Réglages**, section **Application**, tant que l'appli n'est pas installée. Sur iPhone, il explique la marche à suivre dans Safari (Partager, puis Sur l'écran d'accueil).

**Visite guidée :** au premier lancement, quelques écrans présentent la note, les conseils, les courbes et la lumière. Pour la revoir : **Réglages**, section **Application**, **Revoir**.

L'icône SDE apparaît sur l'écran d'accueil et l'application s'ouvre en plein écran. Si la station n'est pas joignable, l'application s'ouvre quand même et propose de se connecter.

Sans le réglage de l'étape 3, **Ajouter à l'écran d'accueil** fonctionne aussi, mais crée un simple raccourci qui s'ouvre dans Chrome.

**Si l'icône du téléphone ouvre Chrome et propose encore d'installer**, Chrome a créé un simple raccourci au lieu d'une vraie application. C'est presque toujours que l'adresse utilisée n'est pas dans le réglage de l'étape 3 (ou que le réglage est resté sur Désactivé). Le téléphone a aussi besoin d'Internet pendant l'installation pour que Chrome fabrique l'application (déduit, pas confirmé par Google) : installe de préférence sur le Wi-Fi de la maison, par `http://sde.local`. Pour corriger : supprime l'icône, vérifie le réglage, puis réinstalle avec le bouton **Installer** de la barre, qui doit ouvrir la fenêtre d'installation de Chrome et non « Ajouter à l'écran d'accueil ». L'onglet **Réglages**, section **Application web progressive** (vue ingénieur), montre si Chrome autorise l'adresse, s'il propose l'installation et si la page est ouverte comme application.

Une installation est liée à une adresse : celle faite sur `192.168.4.1` marche quand le téléphone est sur le Wi-Fi SDE-XXXX. Si tu utilises la station sur le Wi-Fi de la maison, réserve-lui une IP fixe dans ta box pour que l'adresse ne change pas.

## 3 bis. L'appli Android (APK, recommandée sur Android)

Sur Android, Chrome remet sa barre d'adresse par-dessus l'appli installée depuis la station, parce que la station parle en HTTP. L'APK règle ça : une vraie appli plein écran.

1. Envoie `sde.apk` (Releases) sur le téléphone (courriel, Drive, câble...) et ouvre-le.
2. Android demande d'autoriser l'installation depuis cette source (Chrome, Fichiers ou Gmail) : accepte, puis **Installer**. Play Protect peut dire qu'il ne connaît pas l'appli : **Installer quand même**.
3. Ouvre **SDE**. L'appli cherche la station toute seule : `192.168.4.1` (Wi-Fi SDE-XXXX), sa dernière adresse sur le Wi-Fi de la maison, `sde.local`, puis l'accès à distance s'il est réglé.

Mises à jour : rien à faire. Quand tu mets la station à jour (firmware 0.8.8 ou plus récent), l'appli voit le nouveau numéro de version, télécharge la page de la station et l'utilise. L'APK lui-même ne change que si la coquille Android change.

Ce que l'APK fait en plus de la page : il passe par le Wi-Fi même quand celui-ci n'a pas Internet (plus besoin de couper les données mobiles), le bouton retour ferme les fenêtres puis revient à l'accueil, et les exports CSV demandent où enregistrer le fichier. Android 6 ou plus récent.

Pour reconstruire l'APK (sans Android Studio, sous Linux) : `apt install android-sdk-platform-23 aapt zipalign apksigner dalvik-exchange` et un JDK, puis `cd android && ./build.sh <versionCode> <versionName> <firmware de la page> <keystore> <mot de passe>`. Une mise à jour de l'APK doit être signée avec la même clé que la première version, sinon Android refuse de l'installer par-dessus.

## 4. Mettre à jour le firmware par Wi-Fi

**Depuis l'application :** onglet **Réglages**, section **Mise à jour**. Choisis le fichier `sde-firmware-X.Y.Z-ota.bin` (pas le « merged »), entre le mot de passe du Wi-Fi de la station et clique sur **Installer**. La station vérifie le fichier, l'installe, puis redémarre. Réglages et historique sont conservés. Si l'envoi est coupé, l'ancienne version reste en place.

**En ligne de commande** (ordinateur sur le même réseau que la station) : `curl -F "f=@sde-firmware-X.Y.Z-ota.bin" "http://sde.local/api/update?pass=<mot de passe>"`.

## 5. Historique et heure

- **Heure réelle :** quand la station est sur le Wi-Fi de la maison, elle prend l'heure sur Internet (NTP) et la remet à jour régulièrement. Sinon, l'application lui donne l'heure du téléphone ou de l'ordinateur à chaque connexion. Le fuseau est celui du Québec (`TIMEZONE` dans `config.h`).
- **Historique en flash :** la station garde une moyenne par minute pendant 30 jours. Elle l'écrit en flash toutes les 5 minutes, donc une coupure de courant fait perdre au plus 5 minutes. Les minutes mesurées avant de connaître l'heure restent en RAM et sont datées puis écrites dès que l'heure arrive.
- **Graphiques 7 j et 30 j :** l'application relit l'historique de la station, en moyennes de 5 min (7 jours) ou de 15 min (30 jours). La lecture peut prendre une dizaine de secondes : un indicateur de chargement s'affiche sur les graphiques pendant ce temps.
- **Export :** onglet **Réglages**, section **Mes données**, **Télécharger**. Le fichier contient tout l'historique, avec l'heure UTC et l'heure locale, et peut être réimporté dans l'application.
- **Remise à zéro :** **Effacer l'historique**, dans la même section. Le mot de passe du Wi-Fi de la station est demandé.

## 6. Courbes et conseils

- **Lissage des courbes** (case **Lissage** de l'onglet Mesures, en vue ingénieur ; actif par défaut) : un filtre médian sur 5 points enlève les points isolés, puis une moyenne gaussienne sur une fenêtre propre à chaque mesure (CO2 1 min, température 2,5 min, humidité 3 min, indice COV 5 min, pression 10 min), plus longue quand la période affichée s'allonge. L'axe vertical fait au moins quelques fois la précision du capteur (humidité 8 %, température 2 °C, CO2 200 ppm, COV 20, pression 6 hPa, particules 10 µg/m³) : une variation de quelques dixièmes reste plate. Code : `SMOOTH_S`, `MIN_SPAN`, `downsample()`.
- **Valeurs affichées « maintenant »** : médiane des 90 dernières secondes.
- **Conseils pour toi** (l'essentiel sur l'Accueil, tout dans Mesures > Analyse et conseils) : calculés sur les dernières 24 h minute par minute et, pour les habitudes, sur 7 jours en moyennes de 10 min. Pour chaque mesure, l'appli compte le temps passé au-delà de chaque seuil, le plus long épisode, les heures où ça arrive, puis croise les mesures pour trouver la cause et le geste qui va avec. Exemples : CO2 élevé surtout la nuit (chambre fermée), vitesse de baisse du CO2 quand tu aères (renouvellements d'air par heure, d'où la durée d'aération conseillée), air sec en hiver avec CO2 très bas (trop de ventilation) ou avec un chauffage élevé (humidité qu'on aurait à 21 °C), humidité haute avec ou sans CO2 haut, hausses brusques d'humidité (douche, cuisson), point de rosée et buée, pics de particules avec baisse de l'indice COV (cuisson) ou surtout en PM10 (poussière), particules hautes sans pic (pollution extérieure), chute rapide de la pression. Seuils : EN 16798-1, AQI de l'EPA, Santé Canada (CO2 < 1000 ppm sur 24 h), OMS 2021 (PM2.5 < 15 µg/m³ sur 24 h), 5000 ppm (limite au travail), humidex d'Environnement Canada, termes de tendance de pression du Met Office. Code : `buildRecs()` et les fonctions `rec…()` dans `app/sde-analyse.html`.
- Le tableau « Statistiques de la période » (min, moyenne, 95e centile, max) n'apparaît qu'en vue ingénieur.

## 7. La note de qualité de l'air (1 à 5 barres) et ses sources

La note ne contient aucun poids ni réglage maison. Chaque mesure est classée par sa norme publiée, ramenée sur 5 barres (5 = excellent, 1 = très mauvais). La note globale est **la plus basse** des notes, la règle de l'indice AQI de l'EPA, qui retient le pire polluant. Code : `SCALES`, `nowCast()` et `airVerdict()` dans `app/sde-analyse.html`.

| Mesure | 5 barres | 4 | 3 | 2 | 1 | Norme |
|---|---|---|---|---|---|---|
| PM2.5 (µg/m³) | ≤ 9,0 | ≤ 35,4 | ≤ 55,4 | ≤ 125,4 | > 125,4 | AQI de l'EPA, révision du 6 mai 2024 (Bon / Modéré / Malsain pour groupes sensibles / Malsain / Très malsain et Dangereux) |
| PM10 (µg/m³) | ≤ 54 | ≤ 154 | ≤ 254 | ≤ 354 | > 354 | AQI de l'EPA (40 CFR 58, annexe G) |
| CO2 (ppm) | ≤ 970 | ≤ 1220 | ≤ 1770 | ≤ 5000 | > 5000 | EN 16798-1:2019, catégories I à IV (550 / 800 / 1350 ppm au-dessus de l'extérieur, pris à 420 ppm) ; 5000 ppm = limite d'exposition professionnelle sur 8 h (OSHA, ACGIH, RSST au Québec) |
| Humidité (%) | 30 à 50 | 25 à 60 | 20 à 70 | en dehors | – | EN 16798-1:2019, catégories I à IV |

- **Particules :** la valeur notée est la moyenne **NowCast** de l'EPA, celle des cotes publiées heure par heure. Elle pondère les moyennes horaires des 12 dernières heures par w = max(min/max ; 0,5) et demande au moins 2 des 3 dernières heures. Avant cela, l'application prend la moyenne de l'heure écoulée et l'indique comme provisoire.
- **CO2 :** pour comparaison, Santé Canada (2021) recommande moins de 1000 ppm en moyenne sur 24 h.
- **Non comptés :**
  - l'indice COV du BME680, relatif et non calibré, auquel aucune norme ne s'applique ;
  - la pression ;
  - la température, qui relève du confort thermique et non de la qualité de l'air. La tuile Température utilise tout de même les catégories de EN 16798-1 : en saison de chauffe (octobre à avril), 21-23 / 20-24 / 19-25 / 17-26 °C ; l'été, 23,5-25,5 / 23-26 / 22-27 / 21-28 °C.
- **Limites :** les catégories CO2 et humidité de EN 16798-1 servent à la conception des bâtiments. L'AQI vise des moyennes sur 24 h (PM), ici approchées par le NowCast. La station n'est pas un appareil de mesure réglementaire.

## 8. L'interface (version 0.7)

- **Trois onglets :** Accueil, Mesures et Réglages. Sur téléphone, ils sont en bas de l'écran.
- **Une seule échelle de note**, de 5 (excellent) à 1, avec les mêmes couleurs que la bande LED, partout : l'anneau, la bande des 24 dernières heures, les tuiles, la couleur des courbes et les conseils. Chaque note a sa forme d'icône (coche, point d'exclamation, triangle, octogone), pour ne pas dépendre de la couleur seule. Une information sans jugement (pression, pièce fraîche, indice COV) a une icône neutre grise.
- **Accueil :** l'anneau, la phrase d'action, la bande des 24 h (touche une heure pour ouvrir Mesures sur 24 h), quatre tuiles et les conseils les plus utiles. Quand tu aères, l'appli le voit toute seule (le CO2 baisse d'au moins 100 ppm, à 10 ppm par minute ou plus) : elle dit de combien il a baissé et quand refermer, sans bouton à toucher.
- **Mesures :** un bloc par mesure (valeur, état, tendance sur 10 min, graphique dont la courbe prend la couleur de la zone de la norme : un dégradé continu, du vert au orange, et vers le bleu quand la pièce est fraîche), puis COV et pression dans « Plus de mesures », puis l'analyse complète.
- **Nom de la pièce et lumière murale :** dans Réglages. Le nom est gardé par la station (firmware 0.7.0) et par l'appli. La première connexion à une station sans nom ouvre un petit assistant (pièce, Wi-Fi de la maison, lumière).
- **Vue ingénieur** (Réglages > Apparence) : ajoute le nom et la technologie de chaque capteur, le détail de la note (sous-indices, NowCast, norme de chaque mesure), le tableau des statistiques, les périodes 15 min et 6 h, la case Lissage, le point de rosée, et les cartes Connexion, Capteurs, Acquisition (période d'échantillonnage, cycle du SDS011, auto-calibration ABC), Réseau, Stockage, Application web progressive et Journal. Les calculs sont exactement les mêmes dans les deux vues.

## 9. Voir la station de partout (accès à distance, firmware 0.8.0)

La station envoie ses mesures à un serveur MQTT gratuit sur Internet (EMQX Cloud, offre Serverless, sans carte bancaire), par une liaison chiffrée (TLS). L'application s'y connecte depuis n'importe quel réseau : 4G, école, chez des amis. À faire une seule fois :

1. **Installer le firmware 0.9.8** (ou plus récent) : Réglages > Mise à jour, fichier `sde-firmware-0.9.8-ota.bin`. La station doit être connectée au Wi-Fi de la maison (elle a besoin d'Internet).
2. **Créer le serveur gratuit** sur https://www.emqx.com/en/cloud (« Get Started Free ») :
   - **New Deployment**, choisis **Serverless**, une région proche (par exemple AWS N. Virginia), laisse la limite de dépense à 0 pour rester dans l'offre gratuite, puis **Deploy** ;
   - page **Overview** du déploiement : copie l'**adresse** (elle finit par `emqxsl.com`) ;
   - **Access Control > Authentication** : ajoute un identifiant et un mot de passe.
3. **Régler la station**, à la maison : ouvre l'application de la station (`http://sde.local`), puis Réglages > **Accès à distance** > **Voir ma station de partout**. Recopie l'adresse, l'identifiant et le mot de passe, puis **Enregistrer**. L'appli vérifie d'abord les identifiants auprès du serveur. Quelques secondes plus tard, la ligne Station indique « Connectée au serveur ».
4. **Mettre l'application en ligne** (gratuit, GitHub Pages) : dans ce dépôt, **Settings > Pages**, Source **GitHub Actions**. Le workflow `pages.yml` publie le dossier `app/` à chaque modification. L'appli est alors à `https://<ton-pseudo>.github.io/<nom-du-dépôt>/`.
5. **Sur le téléphone**, ouvre cette adresse, entre l'adresse du serveur, ton identifiant et ton mot de passe (pour préremplir l'adresse, renseigne `DEFAULT_CLOUD_HOST` dans `app/sde-analyse.html`), puis installe l'appli (Chrome : menu ⋮ > Installer l'application ; iPhone : Partager > Sur l'écran d'accueil). Comme la page est en HTTPS, l'installation marche sans le réglage `chrome://flags`. Pour un autre appareil (ordinateur de l'école), Réglages > Accès à distance > **Copier le lien pour un autre appareil** (depuis n'importe quel appareil déjà réglé, sauf l'APK) : ouvert sur le nouvel appareil, il le règle sans rien taper ; le lien contient ton mot de passe, envoie-le seulement à toi-même.

L'application de la station (`http://sde.local`) passe aussi toute seule par le serveur quand elle ne trouve pas la station sur le réseau, si elle a été réglée à l'étape 3.

**À distance, tu peux** tout voir (note, mesures, historique 24 h, 7 j et 30 j, conseils) et changer le nom de la pièce, la lumière murale, le cycle du SDS011 ou lancer une mesure. **Sur place seulement :** le Wi-Fi, la mise à jour, la réinitialisation et l'effacement de l'historique. Si la station perd Internet, l'appli le dit et garde les dernières mesures reçues.

**Sécurité :** les identifiants ne sont jamais dans la page en ligne. Ils restent dans la station (mémoire non volatile, jamais renvoyés par l'API) et dans l'appareil qui les a entrés. Le serveur EMQX refuse toute connexion sans identifiant.

**Consommation :** l'offre gratuite d'EMQX donne 1 Go de trafic et 1 million de minutes de connexion par mois. La station connectée en permanence en utilise environ 45 000. Quand personne ne regarde, elle envoie une mesure et son état une fois par minute (environ 60 Mo par mois) ; quand l'appli est ouverte, chaque mesure (environ 2 Mo par heure d'appli ouverte). Ça laisse une bonne marge.

## Si la station redémarre toute seule

Depuis la 0.8.3, la station garde la cause de ses 7 derniers démarrages. L'appli affiche un bandeau quand un démarrage n'était pas voulu, et la vue ingénieur (Réglages > Connexion) montre le détail. Les causes possibles :
- **Chute de tension** (`brownout`) : le chargeur ou le câble ne suit pas, à cause du ventilateur du SDS011, du Wi-Fi et de la lumière. Prends un chargeur d'au moins 1 A et un câble court, ou baisse la lumière.
- **Programme bloqué** (`task_wdt`) ou **plantage** (`panic`) : c'est un bogue du firmware. Branche la station en USB, ouvre un moniteur série à 115200 bauds et envoie la trace affichée au moment du redémarrage.

La version 0.7.1 (Releases) reste disponible pour revenir à la version sans accès à distance, et voir si le problème vient de là.

## À savoir

- **MH-Z19B :** il préchauffe pendant 3 minutes après chaque mise sous tension. Ses valeurs sont alors fausses : le firmware ne les envoie pas, et elles ne sont ni affichées ni enregistrées. L'application affiche « Préchauffage » avec le temps restant.
- **SDS011, durée de vie :** son laser dure environ 8000 h en continu. Le firmware le fait donc dormir : toutes les 5 minutes, il le réveille, fait tourner le ventilateur 30 s pour renouveler l'air, moyenne 10 s de mesures, puis le rendort. Il ne travaille ainsi que ~13 % du temps. Tu peux changer le cycle dans l'application (1, 5, 15 ou 30 min, ou en continu) ou lancer une mesure immédiate.
- **BME680 et SDS011 :** branche-les **ESP32 débranché**. Au redémarrage, le firmware les détecte tout seul (BME680 à l'adresse 0x77 ou 0x76). Tant qu'ils sont absents, l'application l'indique et le reste fonctionne.
- **BME680 « pas détecté » :** depuis la 0.2.3, le firmware vérifie d'abord que SDA et SCL ne sont pas collées au GND, puis scanne le bus I2C et l'application dit ce qu'il a trouvé : rien du tout (câblage, ordre des broches), une autre adresse, ou un module qui est en fait un BME280/BMP280.
- **MH-Z19B qui ne répond pas :** la pastille de l'application indique combien d'octets le capteur a renvoyés. 0 octet veut dire qu'aucun signal n'arrive (alimentation, GND ou fils TX/RX). Si RX et TX sont croisés à l'envers, le firmware les inverse tout seul et l'application le signale dans le journal.
- **Plage du CO2 :** au démarrage, le firmware règle le MH-Z19B sur 0 à 5000 ppm (commande 0x99, `CO2_RANGE_PPM` dans `config.h`). Certains modules sortent d'usine en 0 à 2000 ppm et plafonnent alors à 2000. La carte Capteurs de l'application affiche la plage, relue dans le capteur quand il le permet.
- **Calibration CO2 :** l'auto-calibration (ABC) est désactivée à chaque démarrage, puis son état est relu dans le capteur (commande 0x7D) et affiché dans l'onglet Réglages, section Capteurs (vue ingénieur). Si tu l'actives depuis l'application, elle redevient désactivée au prochain redémarrage. Pour calibrer, laisse la station 20 minutes à l'air extérieur, puis utilise le bouton « Calibrer le CO2 à 400 ppm ».
- **Indice COV :** il est relatif (100 = air le plus propre vu depuis le démarrage). Ce n'est pas l'IAQ calibré de Bosch BSEC, et il lui faut 10 à 30 minutes pour se stabiliser. Si la station passe dehors, l'air froid et propre fixe une référence très haute et l'indice reste bas une fois rentrée, pendant quelques heures. Un redémarrage remet la référence à zéro. Depuis la 0.6.0, la résistance du capteur est lissée sur environ 1 min avant le calcul, et l'humidité et la pression sont suréchantillonnées davantage : beaucoup moins de bruit d'un point à l'autre.
- **Redémarrer et réinitialiser :** onglet **Réglages**, section **Avancé**. **Redémarrer** garde tout. **Réinitialisation d'usine** (mot de passe de la station demandé) efface le Wi-Fi de la maison, le mot de passe du point d'accès (il redevient `sde-station`) et tout l'historique. La station n'est alors joignable que sur son Wi-Fi SDE-XXXX, à `http://192.168.4.1`.

## Ce que fait le firmware

- Un serveur web qui accepte plusieurs connexions à la fois et sert la première qui a envoyé sa requête (`net/pooled_server.*`) : une connexion ouverte d'avance par Chrome sans requête ne bloque plus tout le reste.
- Si le Wi-Fi de la maison est injoignable, les nouveaux essais sont espacés (15 s, 30 s, 1, 2 puis 5 min) : chaque essai balaie les canaux et faisait décrocher le point d'accès.
- Trois tâches FreeRTOS : les capteurs sur le cœur 1, l'USB et le serveur web sur le cœur 0. Les capteurs et le reste sont reliés par une file, et toutes les tâches sont surveillées par le watchdog.
- Le Wi-Fi : un point d'accès toujours actif, plus une connexion optionnelle au Wi-Fi de la maison (identifiants gardés en mémoire non volatile), mDNS `sde.local` et un portail captif.
- Une API HTTP :
  - `GET /api/info`, `GET /api/live?since=<seq>`, `GET /api/log` (journal de la station, en texte)
  - `GET /api/history?since=<epoch>&until=<epoch>&every=<minutes>`, `GET /api/history.csv`, `POST /api/history/reset` (`pass`)
  - `POST /api/time` (`epoch`), `POST /api/update?pass=...` (fichier en multipart)
  - `GET /api/cloud`, `POST /api/cloud` (`host`, `user`, `pass` ; `host` vide = désactiver)
  - `POST /api/cmd` (champ `cmd`), `GET /api/wifi/scan`, `POST /api/wifi` (`ssid`, `pass`), `POST /api/wifi/forget`, `POST /api/wifi/ap` (`old`, `pass`)
- L'accès à distance (`net/cloud.*`) : client MQTT d'ESP-IDF en TLS (port 8883 ; seulement l'intermédiaire « Encryption Everywhere DV TLS CA - G2 » qui signe EMQX, et sa racine DigiCert Global Root G2 : chaque certificat de la liste reste en mémoire pendant la connexion), lancé une fois la station sur Internet. Sujets sous `sde/<SDE-XXXX>/` : `status` (online / offline, retenu, « offline » en testament), `info` (comme `/api/info`, retenu, toutes les minutes), `live` (dernière mesure, retenue ; chaque mesure pendant 90 s après une demande de l'appli, sinon une par minute), `req` (demandes de l'appli : `watch`, `history`, `cmd`, `info`, `log`) et `res/<client>` (réponses ; l'historique arrive en morceaux de 2 ko, espacés de 200 ms car EMQX Serverless limite chaque client à 10 publications par seconde, et s'arrête avec une erreur si la mémoire de la station baisse sous 30 ko). Si la mémoire reste sous 20 ko pendant 20 s, l'accès à distance est coupé 5 min pour garder la station joignable sur place. Les commandes `wifi.*` sont refusées à distance.
- Le journal de la station (`net/event_log.*`) : démarrages et plantages (avec la pile d'appels lue dans la partition coredump), Wi-Fi, accès à distance, demandes d'historique, mises à jour, erreurs et avertissements d'ESP-IDF, mémoire et pile libre des tâches toutes les 10 min. Les lignes passent d'abord par un tampon de 2 ko en RAM RTC, qui survit aux plantages, puis sont écrites en flash toutes les 30 s (`/log.txt`, puis `/log.old` au-delà de 32 ko). Lisible dans l'appli (Réglages, vue ingénieur, Journal de la station) ou à `http://sde.local/api/log`.
- L'historique en LittleFS : un fichier par tranche de 1440 minutes dans `/h/`, écrit seulement en ajout, le plus ancien supprimé au-delà de 30.
- Une moyenne mobile sur 10 échantillons, plus le calcul du point de rosée (Magnus) et de l'humidité absolue.
- Une sortie JSON, une ligne par mesure sur l'USB à 115200 bauds.
- Commandes texte (une par ligne sur l'USB, ou par `POST /api/cmd`) : `info`, `interval <ms>`, `co2.zero`, `co2.abc on|off`, `pm.cycle <s>` (0 = continu, sinon 60 à 3600), `pm.now`, `led <0-100>` (luminosité de la bande LED, 0 = éteinte), `led.night on|off` (bande éteinte de 22 h à 7 h, selon l'heure de la station), `name <texte>` (nom de la pièce, 40 caractères au plus ; `name` seul l'efface), `wifi.set <ssid>|<mot de passe>`, `wifi.forget`

`ts` (heure UTC, en secondes) n'apparaît que lorsque la station connaît l'heure. Les valeurs `pm25` et `pm10` ne sont présentes que dans la ligne qui suit la fin d'un cycle de mesure du SDS011.

Exemple de ligne :
```
{"type":"data","seq":12,"up":24,"ts":1791684000,"co2":{"ok":true,"warm":false,"ppm":612,"avg":605.3,"t":31},"pm":{"ok":true,"state":"sleep","cycle_s":300,"next_s":184},"env":{"ok":true,"addr":119,"t":22.41,"rh":41.2,"p":1013.2,"gas":125.3,"voc":92.1,"dew":8.6,"ah":8.1}}
```

## Brochage utilisé (voir `firmware/src/config.h`)

| Capteur | ESP32 |
|---|---|
| MH-Z19B TX / RX | IO16 (RX) / IO17 (TX) |
| SDS011 TX / RX | IO26 (RX) / IO27 (TX) |
| BME680 SDA / SCL | IO21 / IO22 |
| Bande LED WS2812B (DIN) | IO18, à travers 330 Ω |

## Modifier l'application

Le contenu du dossier `app/` est intégré au firmware sous forme compressée (`firmware/src/web/assets.h`, fichier généré, hors du dépôt). `firmware/build.sh` le régénère avant chaque compilation. À la main :
```
cd firmware
node tools/gen_assets.js
```
Si tu modifies `app/sw.js`, change aussi le nom du cache (`sde-v13` en 0.9.7) pour que les téléphones prennent la nouvelle version.

L'appli en ligne (GitHub Pages) se met à jour toute seule à chaque push qui touche `app/`.

`gen_assets.js` retire l'indentation et les lignes de commentaire de la page avant de la compresser (environ 7 ko de gagnés) : le fichier `app/sde-analyse.html` reste lisible. Le firmware 0.9.8 occupe 99 % de la place réservée au programme (1,25 Mo, table de partitions par défaut). Pour ajouter beaucoup plus, il faudra passer à une table avec un programme plus grand, ce qui demande un flash par USB.

## Prochaines étapes prévues dans le Notion

MQTT avec TLS : fait (section 9, vers EMQX Cloud). Reste InfluxDB et Grafana, qui pourront s'abonner aux mêmes sujets. L'historique en flash sert déjà de tampon : l'appli le relit à distance.
