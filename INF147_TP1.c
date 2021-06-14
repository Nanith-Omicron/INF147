/***************************************************************************************
	FICHIER:    simulation.c
	VERSION:    Version 1
	DATE:    03/06/2021
	AUTEURS:   Chen Jing Tong
				Champfailly Maxime
				Mériaux Théo
				Ismaili Sulaiman
				Melé Bradley
				Phaengvixay Mynon

	DESCRIPTION :   Programme qui fait une simulation afin de déterminer s'il
					est plus rentable pour une usine de déplacer les employés
					ou non afin d'augmenter la production.
***************************************************************************************/


#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<stdbool.h>
#include<string.h>

#include "op_bits.h"     //inclure le module externe à la compilation
#include "mtwister.h"	
#include "chrono.h" 


/***********************CONSTANTES************************/
#define MACRO 0
#define PROB_ABSENCE .03   // 3 % du temps absent sur 1 jour
#define GERME 10	//Germe aleatoire pour initiliser les fonctions de mtwister
#define UN_JOUR 24
#define MIN_EQUIPES 2
#define HEURE_ABSENCE_MAX 8
#define MAX_EQUIPES NB_BITS
#define NB_SIMULATIONS 240000
#define MOYENNE_HEURES_ABSCENCE 8
#define ÉCART_TYPE_HEURES_ABSCENCE 3
#define NB_UNITES_HEURE2 300	// Nombre unites produites par 2 personnes 
#define NB_UNITES_HEURE1 100	// Nombre unites produites par 1 personne
#define NB_PERTES_RELOCALISATION 100 // Nombre unites produites pertes par relocalisation
#define NB_HEURES_CYCLE 24
#define HEURE_RETOUR_TRAVAIL -1

/****************************MACRO-FONCTIONS****************************/
//n est le nombre et i est la position du bit 
#define GET_BIT(n,i) (((i) < NB_BITS) ? (((n) & (1 << (i))) != 0) : 0)

#define SET_BIT(n,i) (((i) < NB_BITS) ? ((n) | (1 << (i))) : n)

#define CLEAR_BIT(n,i) (((i) < NB_BITS) ? ((n) & ~(1 << i)) : n)

#define FLIP_BIT(n,i) (((i) < NB_BITS) ? (GET_BIT(n, i))? (CLEAR_BIT(n, i)): SET_BIT(n, i): (n))


typedef int t_tab_absences[NB_BITS * 2];


/************************************PROTOTYPES******************************/

//Afficher la valeur des moyennes par cycles à l'écran
void afficher_moyenne(long long int nb_unites_avec_reloc, 
	long long int nb_unites_sans_reloc);

//Mettre à jour un employé du tableau d'absences
void update_une_absence(t_tab_absences tab, unsigned int* entier, int i, int j);

//Calculer la moyenne d'unites produites par cycle
long long int calculer_moyenne(long long int somme_total);

//Attribuer un nombre d'heures d'absence un employés
void generer_une_abscence(unsigned int* entier, int i, int j, 
	t_tab_absences tab);

//Permutter la valeur contenu dans deux entiers 
void permuter_nombres(int* nombre1, int* nombre2);

//Relocaliser les travailleurs qui sont seuls dans une équipe
int relocaliser_equipes(t_tab_absences tab, unsigned int* entier1,
	unsigned int* entier2, int nb_equipes);

//Initialise un entier
void initialiser_entier(unsigned int* entier, int nb_equipes);

//Changer le nombre d'équipe par une saisie de l'utilisateur
void saisir_nb_equipes(int* nb_equipes);

//Imprimer les statistiques du cycle présent
void afficher_cycle(unsigned int entier1, unsigned int entier2, 
	long long int total, long long int total_cycle, int nb_cycles);

//Mettre à jour le tableau d'absences
void update_tab_absences(t_tab_absences tab, unsigned int* entier1, 
	unsigned int* entier2, int nb_equipes);

//Calculer les unités produites par les équipes
int calculer_nb_unites(unsigned int entier1, unsigned int entier2,
	int nb_relocalisations);

//Générer les heures d'absences
int generer_hrs_absences(void);

//Attribuer un nombre d'heures d'absence à chacun des employés non absents
void generer_absences(t_tab_absences tab, unsigned int* entier1, 
	unsigned int* entier2, int nb_equipes);


#if 1
int main(void) {

	t_chrono chrono = init_chrono();
	start_chrono(chrono);

	//entiers non signé pour représenter les travailleurs sur les 2 lignes
	unsigned int entier1 = 0,
				 entier2 = 0;

	int nb_equipes,	//nombre d'équipes entre MIN_EQUIPES et MAX_EQUIPES
		nb_relocalisations = 0,
		nb_unites_total = 0,
		nb_unites_heure,
		nb_cycles = 0;

	long long int nb_unites_avec_reloc = 0, //nombres d'unités avec relocalisation
				  nb_unites_sans_reloc = 0, //nombres d'unités sans relocalisation
				  nb_unites_cycle = 0;		//nombres d'unités par cycle

	// table d'absences pour les employés initialisé à 0
	t_tab_absences tab_absences = { 0 }; 

	//Saisir le nombre d’équipes 
	saisir_nb_equipes(&nb_equipes);

	//Mettre la variable de relocalisation à faux
	bool relocalisation = false,
		 affichage = false;

	//Pour 2 tours de boucle (un tour sans relocalisation et un tour avec)
	for (int i = 0; i < 2; i++) {

		//Initialiser le germe aléatoire avec une valeur constante(mt_srand)
		mt_srand(GERME);

		//Initialiser les entiers non-signés à 1.
		initialiser_entier(&entier1, nb_equipes);
		initialiser_entier(&entier2, nb_equipes);

		//Initialiser les compteurs de nombres d'unités
		nb_unites_total = 0;
		nb_unites_cycle = 0;
		nb_cycles = 0;

		//Pour effectuer NB_SIMULATIONS tours de simulations
		for (int i = 1; i <= NB_SIMULATIONS; i++) {
			nb_relocalisations = 0;

			//Générer les absences
			generer_absences(tab_absences, &entier1, &entier2, nb_equipes);

			//Obtenir les unités cumulées (selon qu’il y a relocalisation ou non)
			if (relocalisation) {

				nb_relocalisations = relocaliser_equipes(tab_absences, &entier1, 
					&entier2, nb_equipes);
			}

			nb_unites_heure = calculer_nb_unites(entier1, entier2, nb_relocalisations);

			nb_unites_cycle += nb_unites_heure;
			nb_unites_total += nb_unites_heure;

			/*À tous les NB_HEURES_CYCLE heures, afficher les entiers, les unités 
			produites pour ce cycle
			*/
			if (i % NB_HEURES_CYCLE == 0 && affichage) {

				afficher_cycle(entier1, entier2, nb_unites_total, nb_unites_cycle, 
					++nb_cycles);

				nb_unites_cycle = 0; //Reinitialiser a chaque NB_HEURES_CYCLES
			}

			//Mettre les absences et les entiers non signés à jour
			update_tab_absences(tab_absences, &entier1, &entier2, nb_equipes);

		}
		
		//Prendre en note le nombres d'unités avec et sans relocalisation
		relocalisation ? (nb_unites_avec_reloc = nb_unites_total) : 
			(nb_unites_sans_reloc = nb_unites_total);

		relocalisation = true;

	}

	if (affichage) {
		
		//Afficher la moyenne des unites par cycle avec et sans relocalisation
		afficher_moyenne(nb_unites_avec_reloc, nb_unites_sans_reloc);
	}
	

	stop_chrono(chrono);

	printf("\nLe temps d'execution du programme est de: %lf secondes\n\n\n\n", 
		get_chrono(chrono));

	free_chrono(chrono);

	system("pause");

	return EXIT_SUCCESS;
	
}

#endif


/*******************************************************************************
Générer les heures d'absences
PARAMETRES :  Aucun
RETOUR : Un entier positif aléatoire
SPECIFICATIONS : Appel la function mt_rand_normal de la librairie mt_twister
				 pour générer un entier avec une distribution normale de 
				 moyenne MOYENNE_HEURES_ABSCENCE et un écart-type de 
				 ÉCART_TYPE_HEURES_ABSCENCE 
*******************************************************************************/
int generer_hrs_absences(void) {

	int nb_hrs_abs = 0;

	/*Si la valeur aleatoire generee par mt_rand <= PROB_ABSENCE alors 
		retourner le nombre d'heures d'absences aleatoires generer avec 
		l'ecart-type et la moyenne precise.
	*/
	if (mt_rand() <= PROB_ABSENCE) {

		//valeur de retour de la fonction est un double
		nb_hrs_abs = (int) fabs(mt_rand_normal(MOYENNE_HEURES_ABSCENCE,
			ÉCART_TYPE_HEURES_ABSCENCE));
	}

	return (nb_hrs_abs > HEURE_ABSENCE_MAX) ? HEURE_ABSENCE_MAX : nb_hrs_abs;
}



/*******************************************************************************
Attribue un nombre d'heures d'absence un employés
PARAMETRES :  le tableau d'absences un pointeur entier représentant, la position
			  i dans l'entier et la position j dans le tableau d'absences
RETOUR : VOID
SPECIFICATIONS : Appel la fonction generer_hrs_absences si le bit == 1
				 Appel la fonction generer_une_absence pour chaque donnée du
				 tableau
*******************************************************************************/
void generer_une_abscence(unsigned int* entier, int i, int j, t_tab_absences tab){

#if MACRO
	if (GET_BIT(*entier, i)) {
		tab[j] = generer_hrs_absences();

		if (tab[j]) {

			*entier = CLEAR_BIT(*entier, i);
		}
	}

#else 
	if (get_bit(*entier, i)) { 
		tab[j] = generer_hrs_absences();

		if (tab[j]) {

			*entier = clear_bit(*entier, i);
		}
	}
#endif
}




/******************************************************************************
Attribuer un nombre d'heures d'absence à chacun des employés non absents
PARAMETRES : le tableau d'absences ainsi que les deux pointers entiers
			 représentant les équipes ainsi que le nombre maximal d'équipe
RETOUR : VOID
SPECIFICATIONS : Appel la fonction generer_hrs_absences si le bit == 1
				 Appel la fonction generer_une_absence pour chaque donnée du
				 tableau
*******************************************************************************/
void generer_absences(t_tab_absences tab, unsigned int* entier1, unsigned int* entier2, 
	int nb_equipes) {

	int j;

	for (int i = 0; i < nb_equipes; i++) {
		
		j = i + MAX_EQUIPES;

		generer_une_abscence(entier1, i, i, tab);
		generer_une_abscence(entier2, i, j, tab);


	}
}



/******************************************************************************
Calculer les unités produites par les équipes 
PARAMETRES :  les deux entiers représentant les équipes, nombres de 
			  relocalisations
RETOUR : le nombre d'unité produites par les équipes
SPECIFICATIONS : Si les équipes sont pleinnes, NB_UNITES_HEURE2 est produit sinon, 
				 NB_UNITES_HEURE1 est produites et il y une pertes de 
				 NB_PERTES_RELOCALISATION à chaque relocalisation
*******************************************************************************/
int calculer_nb_unites(unsigned int entier1, unsigned int entier2, 
	int nb_relocalisations) {

	int nb_unites = 0;

	for (int i = 0; i < MAX_EQUIPES; i++) {

#if MACRO
		if (GET_BIT(entier1, i) & GET_BIT(entier2, i)) {

			nb_unites += NB_UNITES_HEURE2;
		}

		else if (GET_BIT(entier1, i) ^ GET_BIT(entier2, i)) {

			nb_unites += NB_UNITES_HEURE1;
		}
#else
		if (get_bit(entier1, i) & get_bit(entier2, i)) {

				nb_unites += NB_UNITES_HEURE2;
		}

		else if (get_bit(entier1, i) ^ get_bit(entier2, i)) {

			nb_unites += NB_UNITES_HEURE1;
		}
#endif
	}


	nb_unites -= nb_relocalisations * NB_PERTES_RELOCALISATION;

	return nb_unites;
}



/******************************************************************************
Mettre à jour un employé du tableau d'absences
PARAMETRES : le tableau d'absences un pointeur entier représentant, la position
			 i dans l'entier et la position j dans le tableau d'absences
RETOUR : VOID

SPECIFICATIONS : Diminuer le nombre d'heure d'absences du tableau de 1 par 
				 heure d'abscence. Si le nombre d'heure d'absence est égal à 0, 
				 remettre l'employé au travail
*******************************************************************************/
void update_une_absence(t_tab_absences tab, unsigned int* entier, int i, int j) {

#if MACRO
	if (!GET_BIT(*entier, i)) {
		if (tab[j]-- == HEURE_RETOUR_TRAVAIL) {

			tab[j] = 0;
			*entier = SET_BIT(*entier, i);
		}
	}
#else
	if (!get_bit(*entier, i)) {
		if (tab[j]-- == HEURE_RETOUR_TRAVAIL) {

			tab[j] = 0;
			*entier = set_bit(*entier, i);
		}
	}
#endif
}



/******************************************************************************
Mettre à jour le tableau d'absences
PARAMETRES : Le tableau d'absence en type t_tab_absences, les pointeurs pour les 
			 équipes représenté par des entiers non signées et le nombre maximum 
			 d'équipe
RETOUR : VOID
SPECIFICATIONS : Diminuer le nombre d'absence du tableau de 1 par cycle. Si le 
				 nombre d'heure d'absence est égal à 0, changer le tableau
*******************************************************************************/
void update_tab_absences(t_tab_absences tab, unsigned int* entier1, unsigned int* entier2, 
	int nb_equipes) {

	int j;

	for (int i = 0; i < nb_equipes; i++) {

		j = i + MAX_EQUIPES;

		update_une_absence(tab, entier1, i, i);
		update_une_absence(tab, entier2, i, j);
	}
}


/******************************************************************************
Imprimer les statistiques du cycle présent.
PARAMETRES : deux entiers, un entier pour le total d'unité produite, un entier 
			 pour le nombre de cycle total, et un entier representant le nombre 
			 de cycle total
RETOUR : VOID
SPECIFICATIONS : Imprime à l'écran des messages préconfigurés
******************************************************************************/
void afficher_cycle(unsigned int entier1, unsigned int entier2, long long int total, 
	long long int total_cycle, int nb_cycles){

	printf("\n/*******Statistiques du %dieme cycle*******/\n", nb_cycles);

	//Afficher les entiers
	printf("Entier1 %s\n", bits2string(entier1));
	printf("Entier2 %s\n", bits2string(entier2));
	
	
	//Afficher les nombres d'unites
	printf("\nIl y a %lld produites pour ce cycle\n", total_cycle);
	printf("\nLe total a date est de %lld unite\n\n\n", total);
}



/******************************************************************************
Changer le nombre d'équipe par une saisie de l'utilisateur
PARAMETRES : un pointer d'entier
RETOUR : VOID

SPECIFICATIONS : Imprime à l'écran, du moment que nb_equipes est plus grand 
				 que 2 et plus petit de 32
******************************************************************************/
void saisir_nb_equipes(int* nb_equipes) {
	do {

		printf("Veuillez entrer le nombre d'equipes (entre %d et %d ): ",
			MIN_EQUIPES, MAX_EQUIPES);

		scanf_s("%d", nb_equipes);

	} while (*nb_equipes > MAX_EQUIPES || *nb_equipes < MIN_EQUIPES );

}



/******************************************************************************
Initialise un entier
 PARAMETRES : un entier non signé, le nombre équipe
 RETOUR : VOID

SPECIFICATIONS : Dépendemment du nombre d'équipe, retourne 0 si Il y a aucune 
				 équipe à cette place
******************************************************************************/
void initialiser_entier(unsigned int* entier, int nb_equipes) {

	for (int i = 0; i < MAX_EQUIPES; i++) {

#if MACRO 
		*entier = (i < nb_equipes) ? SET_BIT(*entier, i) : CLEAR_BIT(*entier, i);
#else
		*entier = (i < nb_equipes) ? set_bit(*entier, i) : clear_bit(*entier, i);
#endif
	}
}


/******************************************************************************
 Relocaliser les travailleurs qui sont seuls dans une équipe
 PARAMETRES : Un tableau avec les absences, 2 entier et le nombre d'équipe
 RETOUR : le nombres de relocalisation
 SPECIFICATIONS : 
	Trouver les positions auxquelles les employés travaillent seul
	Prendre en note une premiere position de la personne travaillant seule, et
	prendre la suivante, et pairer les deux ensembles. Continuer ce processus
	jusqu'à que tout le monde soit en équipe ou qu'il ne reste plus qu'une 
	personne seule.
******************************************************************************/
int relocaliser_equipes(t_tab_absences tab, unsigned int* entier1, 
	unsigned int* entier2, int nb_equipes){

	unsigned int pos_seules; 

	int position = -1,
		position_arrivee,	//position d'arrivee dans le tableau d'absences		
		position_depart,	//position de depart dans le tableau d'absences
		nb_relocalisations = 0;
	
	int i = 0;
	pos_seules = ((*entier1) ^ (*entier2));

	while (pos_seules) {

		if (pos_seules % 2 && position >= 0) { //Si la 1ere personne est trouvee

			

#if MACRO
			//Trouver dans quel entier deplacer la personne seule
			if (GET_BIT(*entier1, position)) {

				*entier2 = SET_BIT(*entier2, position);
				position_arrivee = position + MAX_EQUIPES;
			}

			else {

				*entier1 = SET_BIT(*entier1, position);
				position_arrivee = position;
			}
			//Trouver dans quel entier la personne etait seule
			if (GET_BIT(*entier1, i)) {
				position_depart = i;
				*entier1 = CLEAR_BIT(*entier1, i);
			}

			else {

				position_depart = i + MAX_EQUIPES;
				*entier2 = CLEAR_BIT(*entier2, i);
		}
#else
			//Trouver dans quel entier deplacer la personne seule
			if (get_bit(*entier1, position)){

				*entier2 = set_bit(*entier2, position);
				position_arrivee = position + MAX_EQUIPES;
			}

			else {

				*entier1 = set_bit(*entier1, position);
				position_arrivee = position;
			}

			//Trouver dans quel entier la personne etait seule
			if (get_bit(*entier1, i)) {
				position_depart = i;
				*entier1 = clear_bit(*entier1, i);
			}

			else {

				position_depart = i + MAX_EQUIPES;
				*entier2 = clear_bit(*entier2, i);
			}
#endif

			permuter_nombres(&tab[position_depart], &tab[position_arrivee]);

			position = -1;
			nb_relocalisations++;
		}


		else if (pos_seules % 2) {

			position = i;	//position d'une 1ere personne seule
		}

		i++;
		pos_seules >>= 1;
	}

	return nb_relocalisations;
}



/******************************************************************************
Permutte la valeur contenu dans deux entiers 
PARAMETRES : Les adresses de l'entier 1 et l'entier 2
RETOUR : Aucun
******************************************************************************/
void permuter_nombres(int*nombre1, int*nombre2) {
	int temp;

	temp = *nombre1;
	*nombre1 = *nombre2;
	*nombre2 = temp;

}


/******************************************************************************
Calculer la moyenne d'unites produites par cycle
PARAMETRES: Somme total d'unites produites
RETOUR : La moyenne calculée (long long int)
******************************************************************************/
long long int calculer_moyenne(long long int somme_total) {

	return somme_total* NB_HEURES_CYCLE / NB_SIMULATIONS ;
}



/******************************************************************************
Afficher la valeur des moyennes par cycles à l'écran
PARAMETRES: nombres d'unités produites avec et sans relocalisation
RETOUR : Aucun
******************************************************************************/
void afficher_moyenne(long long int nb_unites_avec_reloc, 
	long long int nb_unites_sans_reloc) {

	printf("\n\n\nMoyenne d'unites produites par cycle sans relocalisation: %lld", 
		calculer_moyenne(nb_unites_sans_reloc));

	printf("\nMoyenne d'unites produites par cycle avec relocalisation: %lld\n\n\n", 
		calculer_moyenne(nb_unites_avec_reloc));
}
