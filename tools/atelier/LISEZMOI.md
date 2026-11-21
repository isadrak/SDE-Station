# Mise en service à l'atelier

`provision.mjs` prépare une station avant de la remettre : accès à distance, alertes, fiche client imprimable.

## Une fois pour toutes, sur EMQX Cloud

Dans Access Control › Authorization, trois règles « All Users », dans cet ordre :

| Sujet | Action | Accès |
|---|---|---|
| `sde/${username}/#` | pubsub | allow |
| `sde/+/classement` | subscribe | allow |
| `#` | pubsub | deny |

Chaque identifiant porte le nom de sa station (`SDE-3F2A`) et ne voit que `sde/SDE-3F2A/…`, plus les résumés du classement CO2 des autres stations (`sde/<station>/classement` : pseudo et moyennes de CO2, publiés seulement par les stations qui participent). Personne ne peut publier sur le sujet d'une autre station. Si l'abonnement est refusé, l'onglet Classement de l'appli l'indique. La station et l'appli du client utilisent le même identifiant. La documentation d'EMQX Cloud confirme ce type de règle avec `${username}`, mais pas explicitement sur l'offre Serverless gratuite : à vérifier dans la console avant la première station.

## Pour chaque station

1. Flasher le firmware 0.10.1 ou plus par USB (image `-merged.bin` à l'adresse 0x0).
2. Connecter l'ordinateur au Wi-Fi de la station (`SDE-XXXX`, mot de passe `sde-station`).
3. `npm install` (une fois), puis :

   ```
   node provision.mjs --host <deployment>.emqxsl.com --alerts
   ```

4. Dans EMQX › Access Control › Authentication, ajouter l'utilisateur affiché (ou importer `sortie/emqx-utilisateurs.csv`).
5. Imprimer `sortie/fiche-SDE-XXXX.html` et la glisser dans la boîte.

`sortie/` contient les mots de passe de toutes les stations : il n'est pas versionné, à sauvegarder ailleurs. `--dry-run` produit une fiche d'essai sans station.
