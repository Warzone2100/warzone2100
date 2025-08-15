; SPDX-License-Identifier: MIT
;
; *** Warzone 2100 Installer extra messages ***
;

[CustomMessages]

; *** Components:

CoreGame=Fichiers de base du jeu
Addons=Extensions
HQTerrain=Terrain haute qualité
AddonMusic=Musiques supplémentaires
AddonCampaigns=Campagnes supplémentaires
Videos=Vidéos
EnglishHQ=Anglais (Haute Qualité)
EnglishStd=Anglais (Qualité Standard)
EnglishLQ=Anglais (Basse Qualité)
KeepExisting=Conserver les fichiers existants
MSSysLibraries=DLLs Microsoft Runtime importantes

; *** Advanced Options

AdvancedButton=&avancé

; *** wzadvancedoptionsform.iss:

AdvancedOptionsTitle=Options avancées - %1
StandardInstall=Installation standard
StandardInstallDesc=Installer %1 sur l'ordinateur (recommandé)
StandardUpdate=Installation standard (mise à jour)
StandardUpdateDesc=Mettre à jour les fichiers existants de %1 sur l'ordinateur (recommandé)
SideBySideInstall=Installation côte à côte
SideBySideInstallDesc=Installe cette version du jeu en plus des versions déjà existantes, y compris l'installation standard. Les paramètres et sauvegardes seront partagées.
PortableInstall=Installation "portable"
PortableInstallDesc=Installe le jeu sur une clé USB, un dossier quelconque, etc. en créant un dossier indépendant, incluant les paramètres et les sauvegardes.
InstallArchitecture=Architecture cible de l'installation :

; *** Various Failure Messages:

UnableToDetermineArchPrompt=Impossible de trouver une architecture compatible. Warzone 2100 peut ne pas supporter votre architecture système. Poursuivre et choisir %1 par défaut ?
VideoDownloadErrorTitle=Erreur de téléchargement des vidéos du jeu
VideoDownloadErrorRetryPrompt=Une erreur est survenue pendant le téléchargement des vidéos du jeu. Réessayer ?
VideoDownloadFailedTitle=Impossible de télécharger les vidéos du jeu
VideoDownloadFailedText=L'installation va se poursuivre, mais vous pourriez vouloir installer les séquences vidéos des campagnes plus tard.

; *** vcredist.iss:

VCRuntimeInstalling=Installation de Visual C++ Runtime Redistributable
VCRuntimeWaiting=En attente de la fin de l'installation...
VCRuntimeVerifying=Vérification du téléchargement

