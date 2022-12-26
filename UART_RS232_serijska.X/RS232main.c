/* 
 * File:   RS232main.c
 * Author: dr Vladimir Rajs
 
Naziv programa: UART serijska RS232 komunikacija
Primena programa:  Program omogucava komunikaciju dsPIC30f4013 sa PC-em preko
                serijskog porta.Brzina komunikacije je 9600 
			    a, frekvencija oscilatora je 10MHz sa PLL-om 4 
Opis programa:  vraca nazad poslati podatak sa mikrokontrolera 

 *
 * Created on 27. septembar 2017., 13.59
 */


#include <stdio.h>
#include <stdlib.h>
#include <p30fxxxx.h>
#include <string.h>
#include <time.h>

_FOSC(CSW_FSCM_OFF & XT_PLL4);//instruction takt je isti kao i kristal
_FWDT(WDT_OFF);

unsigned char tempRX;
unsigned int i, j ,n, hit, r, duzina;
int buf;
//char grad[];
char otkriveno[20];
int zivoti=6;

char grad[10][20]={ //lista gradova
    "london", "sarajevo", "amsterdam", 
    "milan", "hag", "nis", "kragujevac", 
    "kraljevo", "skurbla", "pariz"
};

void initUART1(void)
{
    U1BRG=0x0040;//ovim odredjujemo baudrate

    U1MODEbits.ALTIO=1;//biramo koje pinove koristimo za komunikaciju osnovne ili alternativne

    IEC0bits.U1RXIE=1;//omogucavamo rx1 interupt

    U1STA&=0xfffc;

    U1MODEbits.UARTEN=1;//ukljucujemo ovaj modul

    U1STAbits.UTXEN=1;//ukljucujemo predaju
}


void __attribute__((__interrupt__)) _U1RXInterrupt(void) 
{
    IFS0bits.U1RXIF = 0;
    tempRX=U1RXREG;
    
    if(tempRX >= 'a' && tempRX <= 'z' ){ //proverava da li je uneto slovo
        buf=tempRX; //smesta u buf promenljivu ako jeste
    };
}


/*********************************************************************
* Ime funkcije      : WriteUART1                                     *
* Opis              : Funkcija upisuje podatak u registar 			 *
* 					  za slanje podataka U1TXREG,   				 *
* Parametri         : unsigned int data-podatak koji treba poslati   *
* Povratna vrednost : Nema                                           *
*********************************************************************/

void WriteUART1(unsigned int data)
{
	  while(!U1STAbits.TRMT);

    if(U1MODEbits.PDSEL == 3)
        U1TXREG = data;
    else
        U1TXREG = data & 0xFF;
}

void RS232_putst(register const char *str)
{
  while((*str)!=0)
  {
    WriteUART1(*str);
    str++;
  }
}

int main(int argc, char** argv) {
    
    initUART1();
    //r=9;
    r = rand() % 10;//TO_DO ne daje rand broj jer mikrokontroler uvek ima isti seed, promeniti na ucitavanje ADC signala sa nepovezanog pina
    for (duzina = 0; grad[r][duzina] != '\0'; duzina++); //racunanje duzina stringa
    for(i=0; i<duzina; i++){
        //otkriveno[i]=grad[r][i]; //da otkriveno bude odabrani grad
        otkriveno[i]='_';//da otkriveno bude niz __ duzine odabranog grada
    };
    

	while(1)
	{
        RS232_putst(otkriveno);
        WriteUART1(13);//novi red
        
        while(buf==0);//cekanje dok korisnik ne unese nesto u serijsku komunikaciju
        hit=1;//flag za oduzimanje zivota, po defaultu treba da oduzme
        for(i=0; i<duzina; i++){
            if(buf==grad[r][i]){
                otkriveno[i]=grad[r][i];//otkrivanje slova
                hit=0;//ako je pronadjeno slovo onda spusti flag
            }
        }
        
        if(hit==1)zivoti--;
        
        if(zivoti==0){//ako je GAMEOVER onda uradi ispis
            RS232_putst("GAME OVER");
            WriteUART1(13);//novi red
            RS232_putst("Resenje: ");
            RS232_putst(grad[r]);
            WriteUART1(13);//novi red
            WriteUART1(13);//novi red
            zivoti=6;
            //r=9;
            r = rand() % 10;//TO_DO ne daje rand broj jer mikrokontroler uvek ima isti seed, promeniti na ucitavanje ADC signala sa nepovezanog pina
            for (duzina = 0; grad[r][duzina] != '\0'; duzina++); //racunanje duzina stringa
            for(i=0; i<duzina; i++){
                //otkriveno[i]=grad[r][i]; //da otkriveno bude odabrani grad
                otkriveno[i]='_';//da otkriveno bude niz __ duzine odabranog grada
            };
        }
        buf=0;//praznjenje buf regisra
        
    }//od whilea

    return (EXIT_SUCCESS);
}