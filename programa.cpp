/* ------------------------- SUDOKU NxN ----------------------- */

#include <ga/GASimpleGA.h> //  Algoritmo Genético Simple
#include <ga/GA1DArrayGenome.h> // Genoma --> array de enteros (dim. 1) alelos

#include <string.h>  // Función memset
#include <math.h>  //sqrt()

#include "stdlib.h"
#include <iostream>
#include <fstream>
using namespace std;

int N = 0;
int repeticionesCuadrante(GAGenome& g, int inicio, int tamSubcuadrante);
void mostrarSudoku(struct plantilla *S);
void mostrarSudoku(const GAGenome& g);

float Objective(GAGenome &); // Función objetivo
GABoolean Termina(GAGeneticAlgorithm &); // Función de terminación

void InicioSudoku(GAGenome& g);
int CruceSudoku(const GAGenome& p1, const GAGenome & p2, GAGenome* c1, GAGenome* c2);
int MutacionSudoku(GAGenome& g,float pmut);

struct plantilla{
    int tam;
    int *fijo;
};

void leerSudoku(struct plantilla *S,char *nombreF){
   ifstream f(nombreF);

   f>>S->tam;
   N = S->tam;

   S->fijo = new int[S->tam*S->tam];

   for(int i=0;i<S->tam*S->tam;i++)
           f>>S->fijo[i];

   f.close();
}


int main(int argc, char **argv)
{

    struct plantilla S;
    char * fichero = argv[1];
    leerSudoku(&S, fichero);

    cout << fichero << " Sudoku " << N << "x"<< N << endl << endl;

    mostrarSudoku(&S);

    cout << endl;

// Declaramos variables para los parámetros del GA y las inicializamos

    int popsize = atoi(argv[2]);
    int idSelector = atoi(argv[3]); // indica el número que le hemos asociado al tipo de selector:   1)-Ruleta   2)-Torneo
    int ngen = 12000; //parámetro fijo
    float pcross = atof(argv[4]);  // más cercana a 1
    float pmut = atof(argv[5]); // más cercana a 0

    cout << "Par\240metros:    - Tama\xA4o poblaci\242n: " << popsize << endl;
    cout << "               - N\243mero de generaciones: " << ngen << endl;
    cout << "               - Probabilidad cruce: " << pcross << endl;
    cout << "               - Probabilidad mutaci\242n: " << pmut << endl << endl;


// Conjunto enumerado de alelos --> valores posibles de cada gen del genoma

    GAAlleleSet<int> alelos;
    for(int i=0;i<N;i++) alelos.add(i+1); // del 1 al N


// Creamos el genoma y definimos operadores de inicio, cruce y mutación

    GA1DArrayAlleleGenome<int> genome(N*N,alelos, Objective, &S);
    genome.initializer(InicioSudoku);
    genome.crossover(CruceSudoku);
    genome.mutator(MutacionSudoku);

// Creamos el algoritmo genético

    GASimpleGA ga(genome);

// Inicializamos - minimizar funcion objetivo, tamaño poblacion, nº generaciones,
// pr. cruce y pr. mutación, selección y le indicamos que evolucione.

    ga.minimaxi(-1);
    ga.populationSize(popsize);
    ga.nGenerations(ngen);
    ga.pCrossover(pcross);
    ga.pMutation(pmut);

//Elegimos el tipo de selector según la entrada
    int ruleta = 1;
    int torneo = 2;

    if (idSelector == ruleta) {
        GARouletteWheelSelector selector;
        ga.selector(selector);
    } else if (idSelector == torneo) {
        GATournamentSelector selector;
        ga.selector(selector);
    }

    ga.terminator(Termina);
    ga.evolve(1);

// Imprimimos el mejor individuo que encuentra el GA y su valor fitness

    cout << "El GA encuentra la soluci\242n :\n" <<  endl;
    mostrarSudoku(ga.statistics().bestIndividual());
    cout << endl << "\ncon valor fitness " << ga.statistics().minEver() << endl;

}

// Función objetivo.
float Objective(GAGenome& g) {

    GA1DArrayGenome<int> & genome= (GA1DArrayGenome<int> &) g;

    int tamTotalS = N*N;
    int rep = 0;

    for (int fila=0; fila < N; fila++){

        // utilizamos un array para contar las repeticiones en una fila
        int repFila[N];
        memset(repFila, 0, sizeof(repFila));

        // de la misma manera para la columna
        int repColumna[N];
        memset(repColumna, 0, sizeof(repColumna));


        // Calculamos las repeticiones en la fila
        for(int i=0 + fila*N; i< N + fila*N; i++){ //recorre los elementos de cada fila: fila*S->tam va aumentando de N en N que representa cada fila
             repFila[genome.gene(i) -1] += 1;  // con -1 lo ajustamos a la posición que le corresponde en el array, que va de 0 a N-1
        }
        for (int j=0; j < N; j++){
            if (repFila[j] > 1) rep += repFila[j] - 1; // cuenta aquellos que aparece más de una vez, si en una fila hay tres 7, entonces suma 2 repeticiones.
        }

        // Calculamos las repeticiones en la columna
        for(int i=fila; i < tamTotalS; i+=N) { //recorre los elementos de cada columna: posición actual + tamaño de una fila (N)
            repColumna[genome.gene(i) -1] += 1;  // igual que con las filas
        }

        for (int j=0; j < N; j++) {
            if (repColumna[j] > 1) rep += repColumna[j] -1;  // igual que con las filas
        }

    }

    //Calculamos las repeticiones en cada subcuadrante
    int tamSubcuadrante = sqrt(N);
    int posInicialSubcuadrante = 0;
    int cambioFilaCuadrante = 0; // en un sudoku 9x9 hay 3 filas de subcuadrantes, nos servirá para saber cuando pasamos a la siguiente

    // una iteración por cada subcuadrante, empezamos por el primero (c=0)
    for (int c=0; c < N; c++){

        //comprobamos si tenemos que cambiar de subcuadrante, en ese caso se calcula el siguiente
        if (cambioFilaCuadrante == tamSubcuadrante) {
            cambioFilaCuadrante = 0;
            posInicialSubcuadrante += N*(tamSubcuadrante-1);  //posición inicial en el array de 1D de cada subcuadrante
        }

        // la función calcula todas las repeticiones para ese subcuadrante
        rep += repeticionesCuadrante(genome, posInicialSubcuadrante, tamSubcuadrante); // el segundo parámetro es la posición del primer valor del cuadrante

        posInicialSubcuadrante += tamSubcuadrante; // avanza el puntero al inicio del siguiente subcuadrante
        cambioFilaCuadrante++;
    }


    return rep;

}

int repeticionesCuadrante(GAGenome& g, int inicio, int tamSubcuadrante){
    GA1DArrayGenome<int> & genome= (GA1DArrayGenome<int> &) g;

    int rep = 0;
    int auxInicio = inicio;

    // array para guardar las repeticiones
    int repCuadrante[N];
    memset(repCuadrante, 0, sizeof(repCuadrante));

    for (int i = inicio; i < inicio + N*(tamSubcuadrante-1) + tamSubcuadrante; i++){  //la condición de terminación indica el último elemento en un subcuadrante,  que es recorrer todas las filas y después de la penúltima solo sumamos el tamaño del subcuadrante
        // guarda las repeticiones
        repCuadrante[genome.gene(i) -1] += 1;

        //calcula la siguiente posición en el array de la siguiente subfila
        if (i == auxInicio + (tamSubcuadrante-1)){ // si ha leido toda la subfila del subcuadrante (auxInicio +  (tamSubcuadrante-1)) indica donde termina la subfila
            i = i + N - (tamSubcuadrante); // pasa a la siguiente
            auxInicio = i +1; // auxInicio solo mantiene el inicio de cada subfila
        }
    }

    for (int j=0; j < N; j++) {
        if (repCuadrante[j] > 1) rep += repCuadrante[j] - 1;  // igual que con filas y columnas
    }

    return rep;
}



void mostrarSudoku(struct plantilla *S){

   for(int i=0;i<N*N;i++){
        cout << S->fijo[i] << " ";
        if ((i%N + 1) == N) cout << "\n";
   }
}

void mostrarSudoku(const GAGenome& g){
   const GA1DArrayGenome<int> & genome= (GA1DArrayGenome<int> &) g;
   for(int i=0;i<N*N;i++){
        cout << genome.gene(i) << " ";
        if ((i%N + 1) == N) cout << "\n";
   }
}

void InicioSudoku(GAGenome& g){

    GA1DArrayGenome<int> & genome= (GA1DArrayGenome<int> &) g;

     struct plantilla * plantilla1;
     plantilla1 = (struct plantilla *) genome.userData();


     int aux[plantilla1->tam];

     for(int f=0;f<plantilla1->tam;f++){

          for(int j=0;j<plantilla1->tam;j++) aux[j]=0;

            //añade valores aleatorios a los huecos con un 0
          for(int j=1;j<=plantilla1->tam;j++){
            int v=GARandomInt(0,plantilla1->tam-1);
            while (aux[v]!=0) v=(v+1)%plantilla1->tam;
            aux[v]=j;
          }

          int i=0;

          while(i<plantilla1->tam){

              while((plantilla1->fijo[(f*plantilla1->tam)+i]==0) && (i<plantilla1->tam)) i++;

              if (i<plantilla1->tam){

                     bool encontrado=false;
                     for(int j=0;(j<plantilla1->tam) && (!encontrado);j++)
                             if (aux[j]==plantilla1->fijo[(f*plantilla1->tam)+i]) {
                                encontrado=true;
                                aux[j]=aux[i];
                             }

                     aux[i]=plantilla1->fijo[(f*plantilla1->tam)+i];
              }
              i++;

          }

          for(int c=0;c<plantilla1->tam;c++)
                  genome.gene((f*plantilla1->tam)+c,aux[c]);
     }


}

int CruceSudoku(const GAGenome& p1, const GAGenome & p2, GAGenome* c1, GAGenome* c2){

    const GA1DArrayGenome<int> &m= (const GA1DArrayGenome<int> &) p1;
    const GA1DArrayGenome<int> &p= (const GA1DArrayGenome<int> &) p2;

    struct plantilla * plantilla1 = (struct plantilla *) m.userData();
    int n=0;

    int punto1=GARandomInt(0,m.length());
    while ((punto1%plantilla1->tam)!=0) punto1++;
    int punto2=m.length()-punto1;

    if (c1){

        GA1DArrayGenome<int> &h1= (GA1DArrayGenome<int> &) *c1;

        h1.copy(m,0,0,punto1); // el metodo copy esta definido en la clase GA1DArrayGenome
        h1.copy(p,punto1,punto1,punto2);
        n++;
    }

    if (c2){

        GA1DArrayGenome<int> &h2= (GA1DArrayGenome<int> &) *c2;

        h2.copy(p,0,0,punto1);
        h2.copy(m,punto1,punto1,punto2);
        n++;
    }

    return n;
}


bool checkColumna(int col[], int * check, int tam){
     bool repe=false;

     for(int i=0;i<tam;i++) check[i]=0;

     for(int i=0;i<tam;i++)
             check[col[i]-1]++;
     for(int i=0;i<tam;i++) if (check[i]>1) repe=true;

     return repe;
}


int MutacionSudoku(GAGenome& g, float pmut){

    GA1DArrayGenome<int> & genome= (GA1DArrayGenome<int> &) g;

    struct plantilla * plantilla1;
    plantilla1 = (struct plantilla *) genome.userData();
    int nmut=0;
    int aux; //aux 1
    int fil;
    bool fila;

    int caux[plantilla1->tam];
    int *checkC=new int[plantilla1->tam];

    if (pmut<=0.0) return 0;

    for(int f=0; f<plantilla1->tam; f++)
       for(int c=0; c<plantilla1->tam; c++)
          if (plantilla1->fijo[(f*plantilla1->tam)+c]==0){
           if (GAFlipCoin(pmut) ){
                if (GAFlipCoin(0.5)) fila = true;
                else fila = false;

                if (!fila){

                      for(int j=0;j<plantilla1->tam;j++) caux[j]=genome.gene((j*plantilla1->tam)+c);
                      if (checkColumna(caux,checkC,plantilla1->tam)){
                         int v1 = GARandomInt(0,plantilla1->tam-1);
                         while (checkC[v1]<=1) v1=(v1+1)%plantilla1->tam;
                         v1++;
                         int v2 = GARandomInt(0,plantilla1->tam-1);
                         while (checkC[v2]!=0) v2=(v2+1)%plantilla1->tam;
                         v2++;

                         bool encontrado = false;
                         for(int j=0;j<plantilla1->tam && !encontrado;j++)
                                 if ((plantilla1->fijo[j*(plantilla1->tam)+c]==0)&&(genome.gene(j*(plantilla1->tam)+c)==v1)){
                                    encontrado = true;
                                    genome.gene((j*plantilla1->tam)+c,v2);
                                    fil = j;
                                 }

                         int col=(c+1)%plantilla1->tam;
                         while(genome.gene((fil*plantilla1->tam)+col)!=v2) col=(col+1)%plantilla1->tam;
                         if (plantilla1->fijo[(fil*plantilla1->tam)+col]==0) {
                                nmut++;
                                genome.gene((fil*plantilla1->tam)+col,v1);
                         }
                         else {
                              genome.gene((fil*plantilla1->tam)+c,v1);
                         }

                      }

                }
                else{
                   int v1 = (c + 1) %plantilla1->tam;
                   while ((plantilla1->fijo[(f*plantilla1->tam)+v1]!=0)) v1=(v1+1)%plantilla1->tam;
                   aux = genome.gene((f*plantilla1->tam)+c);
                   genome.gene((f*plantilla1->tam)+c,genome.gene((f*plantilla1->tam)+v1));
                   genome.gene((f*plantilla1->tam)+v1,aux);
                   nmut++;
                }
           }
          }

    return nmut;
}


GABoolean Termina(GAGeneticAlgorithm & ga){
    // si encuentra solución termina si no sigue iterando hasta que terminen las generaciones
    if ( (ga.statistics().minEver()== 0) ||
        (ga.statistics().generation()==ga.nGenerations()) ) return gaTrue;
    else return gaFalse;
}
