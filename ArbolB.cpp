// ArbolB.cpp 
//Author: Ernesto Cuadros-Vargas
//#include <iostream.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include "btree.h"

//const char * keys="CDAMPIWNBKEHOLJYQZFXVRTSGU";
const char * keys1 = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";
const char * keys2 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const char * keys3 = "DYZakHIUwxVJ203ejOP9Qc8AdtuEop1XvTRghSNbW567BfiCqrs4FGMyzKLlmn";

const int BTreeSize = 3;

struct CharLongTrait {
    using keyType = char;
    using ObjIDType = long;
};

int main (int argc, char * argv){
       int result, i;
       BTree<CharLongTrait> bt(BTreeSize);
       for (i = 0; keys1[i]; i++)
       {
               cout<<"Inserting "<<keys1[i]<<endl;
               result = bt.Insert(keys1[i], i*i);
               bt.Print(cout);
       }
       bt.Print(cout);
       for (i = 0; keys2[i]; i++)
       {
               cout << "Searching " << keys2[i] << " ";
               long ObjID = bt.Search(keys2[i]);
               if( ObjID != -1 )
                       cout << "Encontrado " << keys2[i] << " ID = " << ObjID << endl;
               else
                       cout <<"No encontrado!" << keys2[i] << endl;
       }
       cout.flush();

       for (i = 0; keys3[i]; i++)
       {
               cout << "Removing " << keys3[i] << " ";
               if( bt.Remove(keys3[i], -1) )
                       cout << keys3[i] << " removido !" << endl;
               else
                       cout <<"No encontré!" << keys3[i] << endl;
               bt.Print(cout);
       }
       bt.Print(cout);
       cout.flush();
       return 1;
}
