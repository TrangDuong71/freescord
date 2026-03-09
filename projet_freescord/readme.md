# Freescord - Serveur de Chat Multithread en C

Freescord est une application client-serveur de messagerie instantanée développée en C, reposant sur le protocole TCP et la gestion de multiples threads (POSIX).

## Fonctionnalités réalisées

### 1. Connexion et identification
- Le serveur attend les connexions sur un port dédié (ex: 4321).
- Dès qu’un client se connecte, il reçoit un message de bienvenue multi-ligne, avec les explications du fonctionnement.
- Le client doit obligatoirement choisir un pseudonyme via la commande : `nickname <votre_pseudo>`
- Si le pseudo ne respecte pas les règles (moins de 17 caractères, pas de `:` inclus, pas déjà utilisé), le serveur renvoie un code d'erreur (1, 2 ou 3 suivi d'un `\r\n`) et demande de recommencer. Le client reste bloqué dans cette boucle tant qu’il n’a pas fourni un pseudo valide.
- Une fois le pseudo accepté (code 0), il est enregistré, et c'est seulement à ce moment-là que l'utilisateur est officiellement ajouté à la liste des clients actifs.

### 2. Échange de messages
- Dès qu’un client possède son pseudo validé, il peut envoyer des messages.
- Chaque message est automatiquement préfixé par le pseudo de l'auteur (ex: `user: bonjour \r\n`) avant d’être diffusé.
- Les messages sont relayés (broadcast) à tous les clients connectés simultanément, et non pas seulement à l’auteur.

### 3. Gestion multi-clients (Multithreading & Synchronisation)
- Chaque client est géré de manière asynchrone par un thread séparé (`pthread`).
- **Thread-safety :** La liste des clients actifs est protégée (via des `Mutex` / verrous) pour éviter les accès concurrents lors de l'ajout ou de la suppression d'un utilisateur.
- Si un client se déconnecte (ou ferme brusquement le terminal), son thread est terminé proprement, le socket est fermé, la mémoire allouée est libérée, et il disparaît de la liste de diffusion de manière sécurisée.

### 4. Client interactif (I/O Multiplexing)
- Le client peut envoyer et recevoir des messages de façon fluide et non-bloquante grâce à l’utilisation de `poll()`, qui écoute simultanément l’entrée standard (`STDIN`) ET le socket réseau.
- Si le serveur coupe la connexion ou crash, le client détecte immédiatement l’erreur, affiche le message *"Serveur déconnecté"* et quitte proprement le programme.

## Compilation
make / make all

## Exécution
Côté serveur: ./srv
Côté client: ./clt 127.0.0.1

## Documentation
Les supports de cours (CM, TD,TP)
La doc de man (socket, poll, pthread...)

