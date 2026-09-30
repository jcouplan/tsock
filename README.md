# tsock

Projet de réseau en C pour manipuler les sockets.

Le programme gère deux modes :
1. Échange direct entre une source et un puits, soit en TCP soit en UDP.
2. Système de boîte aux lettres : un serveur stocke des messages dans des files selon un identifiant, et des clients peuvent déposer ou venir lire leurs messages.

Les messages envoyés contiennent un numéro de séquence suivi d'un motif de caractères.

## Compilation

gcc Tsock.c -o tsock

## Utilisation

Mode direct :
./tsock -p 8000
./tsock -s localhost 8000 -n 5

Pour basculer en UDP, ajouter l'option -u :
./tsock -p -u 8000
./tsock -s -u localhost 8000 -n 10

Mode boîte aux lettres :
./tsock -b 8000
./tsock -e 1 localhost 8000 -n 3
./tsock -r 1 localhost 8000

## Options
-p : mode puits
-s : mode source
-u : active UDP (TCP par défaut)
-n : nombre de messages
-l : taille des messages
-b : mode serveur boîte aux lettres
-e : déposer dans une boîte
-r : lire une boîte