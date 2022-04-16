#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "parser.h"

#define MAX_INSTRUC 100

typedef struct {
    char mnem[15];  // o rotulo
    int codOp;  // o numero de instruccion
} TReg;


void leeParametros(int argc,char *argv[],char asmFilename[],int *outputOn,char binFilename[]);
void cargaTablaMnemonicos(TReg tablaMnem[],int *NtablaMnem);
void iniciaTablaErrores(int errores[]);
void cargaTablaRegistros(char tablaReg[][50]);
void procesa(char **parsed,TReg tablaMnem[],int NtablaMnem,TReg rotulos[],int *NRot,int *error,int *nInst,int errores[]);
void wrHeader(int errorCompilacion,FILE *archBin,int nInst);
void decodifica(char **parsed,int nInst, TReg tablaMnem[],TReg rotulos[],int *cantOp,int *codOp,int *tipoOpA,int *tipoOpB,int *opA,int *opB,int NtablaMnem,int NRot,int *error,int errores[],char tablaReg[][50]);
void decRegGral(char codReg[],int *operando);
void trABin(int cantOp,int codOp,int tipoOpA,int tipoOpB,int opA,int opB,int *instBin);
void wrParsedIns(char **parsed,int nInst,int errores[],int codOp,int instBin);
void preparaParaEscritura(int *instBin);
void codUpper(char *cod,char *codOp);
int anyToInt(char *s, char **out );

int main(int argc, char *argv[]) {
    FILE *arch;
    FILE *archBin;
    char line[256],asmFilename[25],binFilename[25], tablaReg[50][50];
    TReg tablaMnem[50],rotulos[100];
    char **parsed;
    int nInst,instBin, cantOp, codOp, tipoOpA, tipoOpB, opA, opB,NtablaMnem,errorCompilacion = 0,NRot=0,errores[100],outputOn = 0;

    leeParametros(argc,argv,asmFilename,&outputOn,binFilename); // Parametros pasados por consola
    cargaTablaMnemonicos(tablaMnem,&NtablaMnem);   // Crea tabla con codigos de operacion y mnemonico
    iniciaTablaErrores(errores);    // Inicia una tabla de enteros en 0 (indice representa nro de instruccion)
    cargaTablaRegistros(tablaReg);

    if ((arch=fopen(asmFilename,"r")) != NULL) {    // PRIMERA PASADA
        nInst = 0;
        while (fgets(line,256,arch) != NULL) {
            parsed = parseline(line);
            procesa(parsed,tablaMnem,NtablaMnem,rotulos,&NRot,&errorCompilacion,&nInst,errores);    // Guarda rotulos y busca errores
        }
        freeline(parsed);
        fclose(arch);
    }

    if ((arch=fopen(asmFilename,"r")) != NULL) {    // SEGUNDA PASADA
        archBin = fopen(binFilename,"wb");
        wrHeader(errorCompilacion,archBin,nInst);   // Escribe el header en .mv1
        nInst = 0;
        while (fgets(line,256,arch) != NULL) {
            parsed = parseline(line);
            if (!errores[nInst]) {  // Si la instrucción actual no tiene error (de mnemónico)
                decodifica(parsed,nInst,tablaMnem,rotulos,&cantOp,&codOp,&tipoOpA,&tipoOpB,&opA,&opB,NtablaMnem,NRot,&errorCompilacion,errores,tablaReg);
                trABin(cantOp,codOp,tipoOpA,tipoOpB,opA,opB,&instBin);
            } else
                printf("ERROR: Mnemonico %s inexistente o mal escrito\n",parsed[1]);
            if (outputOn)
                wrParsedIns(parsed,nInst,errores,codOp,instBin);   // Imprime
            if (!errorCompilacion) {
                preparaParaEscritura(&instBin);// Pasa de littleEndian a bigEndian
                fwrite(&instBin,4,1,archBin);      // Escribe arch binario (si hubo error no)
            }
            nInst++;
        }
        freeline(parsed);
        fclose(arch);
        fclose(archBin);
        if (errorCompilacion) {
            printf("\nLa traduccion no tuvo exito por la presencia de 1 o mas errores.\n");
            remove(binFilename);
        }
        else
            printf("\nTraduccion exitosa.\n");
    }
    return 0;

}

void leeParametros(int argc,char *argv[],char asmFilename[],int *outputOn,char binFilename[]) {
    unsigned int i;
    char *extFile;
    for (i=1;i<argc;i++)
        if (!strcmp(argv[i],"-o"))
            *outputOn = 1;
        else {
            extFile = argv[i] + strlen(argv[i]) - 4;
            if (!strcmp(extFile,".asm"))
                strcpy(asmFilename,argv[i]);
            else if (!strcmp(extFile,".mv1"))
                strcpy(binFilename,argv[i]);
        }
}

void cargaTablaMnemonicos(TReg tablaMnem[],int *NtablaMnem) {

    strcpy(tablaMnem[0].mnem,"MOV");
    tablaMnem[0].codOp = 0;
    strcpy(tablaMnem[1].mnem,"ADD");
    tablaMnem[1].codOp = 1;
    strcpy(tablaMnem[2].mnem,"SUB");
    tablaMnem[2].codOp = 2;
    strcpy(tablaMnem[3].mnem,"SWAP");
    tablaMnem[3].codOp = 3;
    strcpy(tablaMnem[4].mnem,"MUL");
    tablaMnem[4].codOp = 4;
    strcpy(tablaMnem[5].mnem,"DIV");
    tablaMnem[5].codOp = 5;
    strcpy(tablaMnem[6].mnem,"CMP");
    tablaMnem[6].codOp = 6;
    strcpy(tablaMnem[7].mnem,"SHL");
    tablaMnem[7].codOp = 7;
    strcpy(tablaMnem[8].mnem,"SHR");
    tablaMnem[8].codOp = 8;
    strcpy(tablaMnem[9].mnem,"AND");
    tablaMnem[9].codOp = 9;
    strcpy(tablaMnem[10].mnem,"OR");
    tablaMnem[10].codOp = 10;
    strcpy(tablaMnem[11].mnem,"XOR");
    tablaMnem[11].codOp = 11;

    strcpy(tablaMnem[12].mnem,"SYS");
    tablaMnem[12].codOp = 240;
    strcpy(tablaMnem[13].mnem,"JMP");
    tablaMnem[13].codOp = 241;
    strcpy(tablaMnem[14].mnem,"JZ");
    tablaMnem[14].codOp = 242;
    strcpy(tablaMnem[15].mnem,"JP");
    tablaMnem[15].codOp = 243;
    strcpy(tablaMnem[16].mnem,"JN");
    tablaMnem[16].codOp = 244;
    strcpy(tablaMnem[17].mnem,"JNZ");
    tablaMnem[17].codOp = 245;
    strcpy(tablaMnem[18].mnem,"JNP");
    tablaMnem[18].codOp = 246;
    strcpy(tablaMnem[19].mnem,"JNN");
    tablaMnem[19].codOp = 247;
    strcpy(tablaMnem[20].mnem,"LDL");
    tablaMnem[20].codOp = 248;
    strcpy(tablaMnem[21].mnem,"LDH");
    tablaMnem[21].codOp = 249;
    strcpy(tablaMnem[22].mnem,"RND");
    tablaMnem[22].codOp = 250;
    strcpy(tablaMnem[23].mnem,"NOT");
    tablaMnem[23].codOp = 251;

    strcpy(tablaMnem[24].mnem,"STOP");
    tablaMnem[24].codOp = 4081;
    *NtablaMnem = 25;
}

void iniciaTablaErrores(int errores[]) {
    unsigned int nInst;
    for (nInst=0;nInst<MAX_INSTRUC;nInst++)
            errores[nInst] = 0;
}

void cargaTablaRegistros(char tablaReg[][50]) {
    strcpy(tablaReg[0],"DS");
    strcpy(tablaReg[1],"");
    strcpy(tablaReg[2],"");
    strcpy(tablaReg[3],"");
    strcpy(tablaReg[4],"");
    strcpy(tablaReg[5],"IP");
    strcpy(tablaReg[6],"");
    strcpy(tablaReg[7],"");
    strcpy(tablaReg[8],"CC");
    strcpy(tablaReg[9],"AC");
}

void procesa(char **parsed,TReg tablaMnem[],int Ntabla,TReg rotulos[],int *NRot,int *error,int *nInst,int errores[]) {
    char mnem[6],rotulo[15];
    int i;

    // DECODIFICA LABEL Y GUARDA SU CORRESPONDIENTE NRO DE INSTRUCCION
    if (parsed[0]) {    // Si tiene rótulo
        codUpper(parsed[0],rotulo);
        strcpy(rotulos[*NRot].mnem,rotulo);
        rotulos[*NRot].codOp = *nInst;
        (*NRot)++;
    }

    // DECODIFICA MNEMONICO EN BUSCA DE ERROR
    codUpper(parsed[1],mnem);
    i = 0;
    while (i<Ntabla && strcmp(mnem,tablaMnem[i].mnem))    // Busca si hay error por mnemonico inexistente
        i++;
    if (i>=Ntabla) {  // Guarda el nro de instruccion en el que hay error (indice con referencia)
        errores[*nInst] = 1;
        *error = 1;
    }
    (*nInst)++;
}

void wrHeader(int errorCompilacion,FILE *archBin,int nInst) {
    int cero = 0;
    if (!errorCompilacion) {                       // HEADER
        fwrite("MV-1",4,1,archBin);    // 4 chars fijos
        preparaParaEscritura(&nInst);
        fwrite(&nInst,4,1,archBin); // Tamaño del codigo (en cantidad de instrucciones)
        fwrite(&cero,4,1,archBin);  // 3 lineas reservadas con 0s
        fwrite(&cero,4,1,archBin);
        fwrite(&cero,4,1,archBin);
        fwrite("V.22",4,1,archBin);    // 4 chars fijos
    }
}

void decodifica(char **parsed,int nInst, TReg tablaMnem[],TReg rotulos[],int *cantOp,int *codOp,int *tipoOpA,int *tipoOpB,int *opA,int *opB,int NtablaMnem,int NRot,int *error,int errores[],char tablaReg[][50]) {
    int i;
    char mnem[6], *out,codReg[5],rotulo[15];

    // DECODIFICA CANTIDAD DE OPERACIONES
    if (!parsed[2])
        *cantOp = 0;
    else if (!parsed[3])
        *cantOp = 1;
    else
        *cantOp = 2;

    // DECODIFICA CODIGO DE OPERACION EN DECIMAL
    codUpper(parsed[1],mnem);
    i = 0;
    while (i<NtablaMnem && strcmp(mnem,tablaMnem[i].mnem))    // Ya se verificó que no halla error
        i++;
    *codOp = tablaMnem[i].codOp;

    // DECODIFICA TIPO DE OPERANDO A Y/O B SEGUN CORRESPONDA EN DECIMAL
    if (*cantOp != 0) {
        if ((parsed[2][0] >= '0' && parsed[2][0] <= '9') || parsed[2][0] == '#' || parsed[2][0] == '@' || parsed[2][0] == '%' || (*codOp >=241 && *codOp<=247)) {   // A inmediato
            *tipoOpA = 0;
        } else if (parsed[2][0] == '[') {   // A directo
            *tipoOpA = 2;
        } else {                   // A de registro
            *tipoOpA = 1;
        }
        if (*cantOp == 2){
            if ((parsed[3][0] >= '0' && parsed[3][0] <= '9') || parsed[3][0] == '#' || parsed[3][0] == '@' || parsed[3][0] == '%' || (*codOp >=241 && *codOp<=247)) {   // B inmediato
                *tipoOpB = 0;
            } else if (parsed[3][0] == '[') {   // B directo
                *tipoOpB = 2;
            } else {                   // B de registro
                *tipoOpB = 1;
            }
        }
    }

    // DECODIFICA OPERANDO A Y/O B SEGUN CORRESPONDA EN DECIMAL
    if (*cantOp != 0) {     // Decodifica operando A
        if (*codOp >=241 && *codOp<=247) {   // Si se trata de una operacion de salto
            codUpper(parsed[2],rotulo);
            i = 0;
            while (i < NRot && strcmp(rotulo,rotulos[i].mnem))
                i++;
            if (i >= NRot) {
                printf("ERROR: El rotulo al cual se quiere saltar no existe\n");
                *error = 1;
                errores[nInst] = 1;
            } else
                *opA = rotulos[i].codOp;
        } else {
            if (*tipoOpA == 1) {  // Si es de TIPO REGISTRO
                codUpper(parsed[2],codReg);
                i=0;
                while (i < 10 && strcmp(codReg,tablaReg[i]))
                    i++;
                if (i < 10 && !strcmp(codReg,tablaReg[i]))  // salió porque lo encontró
                    *opA = i;
                else      // Es un General Purpose Register (decodificacion especial)
                    decRegGral(codReg,opA);
            } else {                // TIPO INMEDIATO o DIRECTO
                if (parsed[2][0] == 39)     // Primero se fija que no se trate de lectura de caracter ASCII (si lo es, guarda su valor numerico dependiendo de si es inmediato o directo)
                    *opA = parsed[2][1];
                else if (parsed[2][1] == 39)
                    *opA = parsed[2][2];
                else if (*tipoOpA == 2)
                    *opA = anyToInt(parsed[2]+1,&out);    // Si es una instruccion en hexa, octal o decimal, la guarda en decimal
                else
                    *opA = anyToInt(parsed[2],&out);
            }
        }

        if (*cantOp == 2) {     // Si tiene operando B hace exactamente lo mismo
            if (*tipoOpB == 1) {
                codUpper(parsed[3],codReg);
                i=0;
                while (i < 10 && strcmp(codReg,tablaReg[i]))
                    i++;
                if (i < 10 && !strcmp(codReg,tablaReg[i]))  // salió porque lo encontró
                    *opB = i;
                else      // Es un General Purpose Register (decodificacion especial)
                    decRegGral(codReg,opB);
            } else {
                if (parsed[3][0] == 39)
                    *opB = parsed[3][1];
                else if (parsed[3][1] == 39)
                    *opB = parsed[3][2];
                else if (*tipoOpB == 2)
                    *opB = anyToInt(parsed[3]+1,&out);
                else
                    *opB = anyToInt(parsed[3],&out);
            }
        }
    }

}

void decRegGral(char codReg[],int *operando) {
    if (strlen(codReg) == 3)     //  4 bytes (porque son 3 caracteres)
        *operando = codReg[1] - 55;   // Resta 55 porque es la forma de pasar el caracter ASCII (hexa) a decimal
    else
        if (codReg[1] == 'X')    //  2 ultimos bytes (porque el 2do caracter es X)
            *operando = codReg[0] - 55 + 48;
        else if (codReg[1] == 'L')   // 4to byte (porque el 2do caracter es L)
            *operando = codReg[0] - 55 + 16;
        else                            // 3er byte (porque el 2do caracter es H)
        *operando = codReg[0] - 55 + 32;
}

void trABin(int cantOp,int codOp,int tipoOpA,int tipoOpB,int opA,int opB,int *instBin) {
    *instBin = 0;   // 32 bits en 0
    if (cantOp == 0)
        *instBin = codOp << 20;
    else if (cantOp == 1) {
        *instBin = (codOp << 24) | ((tipoOpA << 22) & 0x00C00000) | (opA & 0x0000FFFF);
        if (opA>>16)
            printf("WARNING: Truncado de operando. 16 bits insuficientes para guardar el valor del operando.\n");
    } else {
        *instBin = (codOp << 28) | ((tipoOpA << 26) & 0x0C000000) | ((tipoOpB << 24) & 0x03000000) | ((opA << 12) & 0x00FFF000) | (opB & 0x00000FFF);
        if (opB>>12)
            printf("WARNING: Truncado de operando. 12 bits insuficientes para guardar el valor del operando B.\n");
        if (opA>>12)
            printf("WARNING: Truncado de operando. 12 bits insuficientes para guardar el valor del operando A.\n");
    }
}

int anyToInt(char *s, char **out ) {
    char *BASES = {"**$*****@*#*****%"};
    int base = 10;
    char *bp = strchr(BASES,*s);
    if (bp != NULL) {
        base = bp - BASES;
        ++s;
    }
    return strtol(s,out,base);
}

void codUpper(char *cod,char codOp[]) { // Pasa string a mayuscula
    unsigned int i=0;
    while ((cod[i]>= 'a' && cod[i]<='z') || (cod[i]>= 'A' && cod[i]<='Z')) {
        codOp[i] = (cod[i]>=65 && cod[i]<=90) ? (cod[i]) : (cod[i]-32);
        i++;
    }
    codOp[i] = '\0';
}

void wrParsedIns(char **parsed,int nInst,int errores[],int codOp,int instBin) {
    if (nInst < 10)
        printf("[000%d]: ",nInst);
    else if (nInst < 100)
        printf("[00%d]: ",nInst);
    else if (nInst < 1000)
        printf("[0%d]: ",nInst);
    else if (nInst < 10000)
        printf("[%d]: ",nInst);

    if (errores[nInst]) {
        if (codOp>=241 && codOp<=247)   // Error por rótulo no encontrado
            printf("%02X %02X %0XF FF\t",(instBin>>24)&0xFF,(instBin>>16)&0xFF,(instBin>>12)&0xF);
        else                            // Error por mnemónico inexistente
            printf("FF FF FF FF\t");
    } else
        printf("%02X %02X %02X %02X\t",(instBin>>24)&0xFF,(instBin>>16)&0xFF,(instBin>>8)&0xFF,instBin&0xFF);

    if (parsed[0])
        printf("%9s: ",parsed[0]);
    else
        printf("%9d: ",nInst+1);
    printf("%s\t%7s",parsed[1],parsed[2] ? parsed[2]:"");
    if (parsed[3])
        printf(", %s",parsed[3]);
    else
        printf("\t");
    if (parsed[4])
        printf("\t; %s",parsed[4]);
    printf("\n");
}

void preparaParaEscritura(int *instBin) {
    unsigned int b0,b1,b2,b3;
    b0 = ((*instBin) & 0x000000FF) <<24;
    b1 = ((*instBin) & 0x0000FF00) <<8;
    b2 = ((*instBin) & 0x00FF0000) >>8;
    b3 = ((*instBin) & 0xFF000000) >>24;
    *instBin = b0 | b1 | b2 | b3;
}
