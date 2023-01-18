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
#include "driverGLCD.h"
#include "adc.h"
#include <outcompare.h>
#include "timer2.h"
#include "stdint.h"

#define DRIVE_A PORTCbits.RC13
#define DRIVE_B PORTCbits.RC14
#define KEY_NOT_PRESSED (-1)

_FOSC(CSW_FSCM_OFF & XT_PLL4);//instruction takt je isti kao i kristal
//_FOSC(CSW_ON_FSCM_OFF & HS3_PLL4);
_FWDT(WDT_OFF);
_FGS(CODE_PROT_OFF);

uint64_t freq_values[8][2] = { { 5682, 2841 }, { 6378, 3189 }, { 7159, 3579 }, { 7584, 3792 },    // 8 kHz, 4 kHz, 2 kHz, 1 kHz, 
                                       { 8513, 4257 }, { 9556, 4778 }, { 10124, 5062 }, { 11364, 5682 } };  // 500 Hz, 250 Hz, 125 Hz
int key_pressed = KEY_NOT_PRESSED;

unsigned char tempRX;
unsigned int i, j ,n, hit, win, r, r1, r2, duzina;
int buf;
//char grad[];
char otkriveno[20];
int zivoti=6;
int pomoc=3;
unsigned int X, Y,x_vrednost, y_vrednost;

//const unsigned int ADC_THRESHOLD = 900; 
const unsigned int AD_Xmin =220;
const unsigned int AD_Xmax =3642;
const unsigned int AD_Ymin =520;
const unsigned int AD_Ymax =3450;

unsigned int sirovi0,sirovi1,sirovi2,sirovi3;
unsigned int broj,broj1,broj2,temp0,temp1,pir,fr; 

char grad[10][20]={ //lista gradova
    "london", "sarajevo", "amsterdam", 
    "milan", "hag", "nis", "kragujevac", 
    "kraljevo", "subotica", "pariz"
};

void ConfigureTSPins(void)
{
	//ADPCFGbits.PCFG10=1;
	//ADPCFGbits.PCFG7=digital;

	//TRISBbits.TRISB10=0;
	TRISCbits.TRISC13=0;
    TRISCbits.TRISC14=0;
	
	//LATCbits.LATC14=0;
	//LATCbits.LATC13=0;
}

void initUART1(void)
{
    U1BRG=0x0040;//ovim odredjujemo baudrate

    U1MODEbits.ALTIO=0;//biramo koje pinove koristimo za komunikaciju osnovne ili alternativne

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

void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
	sirovi0=ADCBUF0;//0
	sirovi1=ADCBUF1;//1
    sirovi2=ADCBUF2;//0
	sirovi3=ADCBUF3;//1

	pir=sirovi0;
    fr=sirovi1;
    temp0=sirovi2;
	temp1=sirovi3;

    IFS0bits.ADIF = 0;
}

void __attribute__((__interrupt__)) _T2Interrupt(void)
{

   	TMR2 =0;
    IFS0bits.T2IF = 0;
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

void WriteUART1dec2string(unsigned int data)
{
	unsigned char temp;

	temp=data/1000;
	WriteUART1(temp+'0');
	data=data-temp*1000;
	temp=data/100;
	WriteUART1(temp+'0');
	data=data-temp*100;
	temp=data/10;
	WriteUART1(temp+'0');
	data=data-temp*10;
	WriteUART1(data+'0');
}
void Delay(unsigned int N)
{
	unsigned int i;
	for(i=0;i<N;i++);
}

void Touch_Panel (void)
{
// vode horizontalni tranzistori
	DRIVE_A = 1;  
	DRIVE_B = 0;
    
     LATCbits.LATC13=1;
     LATCbits.LATC14=0;

	Delay(500); //cekamo jedno vreme da se odradi AD konverzija
				
	// ocitavamo x	
	x_vrednost = temp0;//temp0 je vrednost koji nam daje AD konvertor na BOTTOM pinu		

	// vode vertikalni tranzistori
     LATCbits.LATC13=0;
     LATCbits.LATC14=1;
	DRIVE_A = 0;  
	DRIVE_B = 1;

	Delay(500); //cekamo jedno vreme da se odradi AD konverzija
	
	// ocitavamo y	
	y_vrednost = temp1;// temp1 je vrednost koji nam daje AD konvertor na LEFT pinu	
	
//Ako želimo da nam X i Y koordinate budu kao rezolucija ekrana 128x64 treba skalirati vrednosti x_vrednost i y_vrednost tako da budu u opsegu od 0-128 odnosno 0-64
//skaliranje x-koordinate

    X=(x_vrednost-161)*0.03629;



//X= ((x_vrednost-AD_Xmin)/(AD_Xmax-AD_Xmin))*128;	
//vrednosti AD_Xmin i AD_Xmax su minimalne i maksimalne vrednosti koje daje AD konvertor za touch panel.


//Skaliranje Y-koordinate
	Y= ((y_vrednost-500)*0.020725);

//	Y= ((y_vrednost-AD_Ymin)/(AD_Ymax-AD_Ymin))*64;
}

void Write_GLCD(unsigned int data)
{
    unsigned char temp;

    temp=data/1000;
    Glcd_PutChar(temp+'0');
    data=data-temp*1000;
    temp=data/100;
    Glcd_PutChar(temp+'0');
    data=data-temp*100;
    temp=data/10;
    Glcd_PutChar(temp+'0');
    data=data-temp*10;
    Glcd_PutChar(data+'0');
}

int main(int argc, char** argv) {
    
    ConfigureLCDPins();
	ConfigureTSPins();
	GLCD_LcdInit();
	GLCD_ClrScr();
    initUART1();
	ADCinit();
	ConfigureADCPins();
	ADCON1bits.ADON=1;
    
    //BUZZER
    TRISDbits.TRISD9=0;//konfigurisemo kao izlaz za pwm 
    PR2= 2; //freq_values[0][0];//odredjuje frekvenciju po formuli
        OC1RS=1; //freq_values[0][1];//postavimo pwm
        OC1R=2000;//inicijalni pwm pri paljenju samo
        OC1CON =OC_IDLE_CON & OC_TIMER2_SRC & OC_PWM_FAULT_PIN_DISABLE& T2_PS_1_256;//konfiguracija pwma
        T2CONbits.TON=1;//ukljucujemo timer koji koristi
        
        
    /*if (key_pressed != (-1))
        {
          PR2   = freq_values[key_pressed][0];//odredjuje frekvenciju po formuli
          OC1RS = freq_values[key_pressed][1];//postavimo pwm
        } else
        {
          PR2   = 2;
          OC1RS = 1;
        }
     */ //za buzzer ukljucivanje
    
    //r=9;
    r1 = rand() % 10;//TO_DO ne daje rand broj jer mikrokontroler uvek ima isti seed, promeniti na ucitavanje ADC signala sa nepovezanog pina
    for (duzina = 0; grad[r1][duzina] != '\0'; duzina++); //racunanje duzina stringa
    for(i=0; i<duzina; i++){
        //otkriveno[i]=grad[r][i]; //da otkriveno bude odabrani grad
        otkriveno[i]='_';//da otkriveno bude niz __ duzine odabranog grada
    };
    pomoc=3;
    

	while(1)
	{
        GLCD_ClrScr();
        RS232_putst(otkriveno);
        WriteUART1(13);//novi red
        GoToXY(40,2);
        GLCD_Printf (otkriveno);
        GoToXY(5,4);
        GLCD_Printf ("Pomoc PIR-a: ");
        for(i=0; i<pomoc; i++){
                GLCD_Printf ("* ");
            }
        GoToXY(5,6);
        GLCD_Printf ("Zivoti: ");
        for(i=0; i<zivoti; i++){
                GLCD_Printf ("<3");
            }
        while(buf==0){//cekanje dok korisnik ne unese nesto u serijsku komunikaciju
            WriteUART1(' ');
            WriteUART1dec2string(sirovi0);
            WriteUART1(' ');
            WriteUART1dec2string(sirovi1);
            WriteUART1(13);//enter

            for(broj1=0;broj1<2000;broj1++)
            for(broj2=0;broj2<500;broj2++);
            
            if(sirovi0 > 2000 && pomoc>0){
                pomoc--;
                j=0;
                for(i=0; i<duzina; i++){//ako nema praznog mesta igrac je pobedio
                    if(otkriveno[i]=='_') j++;
                }
                r2= rand() % j;
                j=0;
                for(i=0; i<duzina; i++){//ako nema praznog mesta igrac je pobedio
                    if(otkriveno[i]=='_'){
                        if (j==r2) otkriveno[i]=grad[r1][i];
                        j++;
                    }
                }
                GLCD_ClrScr();
                RS232_putst(otkriveno);
                WriteUART1(13);//novi red
                GoToXY(40,2);
                GLCD_Printf (otkriveno);
                GoToXY(5,4);
                GLCD_Printf ("Pomoc PIR-a: ");
                for(i=0; i<pomoc; i++){
                    GLCD_Printf ("* ");
                }
                GoToXY(5,6);
                GLCD_Printf ("Zivoti: ");
                for(i=0; i<zivoti; i++){
                    GLCD_Printf ("<3");
                }
            }
        };
        //ako hocemo da otkrijemo celu rec odjednom
        //if(tempRX==grad[r])...
        hit=1;//flag za oduzimanje zivota, po defaultu treba da oduzme
        win=1;
        for(i=0; i<duzina; i++){
            if(buf==grad[r1][i]){
                otkriveno[i]=grad[r1][i];//otkrivanje slova
                hit=0;//ako je pronadjeno slovo onda spusti flag
            }
        }
        for(i=0; i<duzina; i++){//ako nema praznog mesta igrac je pobedio
            if(otkriveno[i]=='_'){
                win=0;//ako nadje prazno mesto spusti zastavicu za pobedu
            }
        }
        if(hit==1)zivoti--;
        
        if(win==1){//ako pogodi sve
            GLCD_ClrScr();
            RS232_putst("POBEDILI STE");
            WriteUART1(13);//novi red
            GoToXY(40,2);
            GLCD_Printf ("POBEDILI STE");
            
            WriteUART1(13);//novi red
            WriteUART1(13);//novi red
            for(i=0; i<10000; i++){
                for(j=0; j<750; j++);
            }
            GLCD_ClrScr();
            for(i=0; i<20; i++){
                otkriveno[i]='\0';
            }
            zivoti=6;
            //r=9;
            r1 = rand() % 10;//TO_DO ne daje rand broj jer mikrokontroler uvek ima isti seed, promeniti na ucitavanje ADC signala sa nepovezanog pina
            for (duzina = 0; grad[r1][duzina] != '\0'; duzina++); //racunanje duzina stringa
            for(i=0; i<duzina; i++){
                //otkriveno[i]=grad[r][i]; //da otkriveno bude odabrani grad
                otkriveno[i]='_';//da otkriveno bude niz __ duzine odabranog grada
            };
        }
        
        if(zivoti==0){//ako je GAMEOVER onda uradi ispis i resetuj
            GLCD_ClrScr();
            RS232_putst("GAME OVER");
            WriteUART1(13);//novi red
            GoToXY(40,2);
            GLCD_Printf ("GAME OVER");
            
            RS232_putst("Resenje: ");
            GoToXY(40,4);
            GLCD_Printf ("Resenje: ");
            RS232_putst(grad[r1]);
            WriteUART1(13);//novi red
            WriteUART1(13);//novi red
            GoToXY(40,5);
            GLCD_Printf (grad[r1]);
            for(i=0; i<10000; i++){
                for(j=0; j<750; j++);
            }
            GLCD_ClrScr();
            for(i=0; i<20; i++){
                otkriveno[i]='\0';
            }
            zivoti=6;
            //r=9;
            r1 = rand() % 10;//TO_DO ne daje rand broj jer mikrokontroler uvek ima isti seed, promeniti na ucitavanje ADC signala sa nepovezanog pina
            for (duzina = 0; grad[r1][duzina] != '\0'; duzina++); //racunanje duzina stringa
            for(i=0; i<duzina; i++){
                //otkriveno[i]=grad[r][i]; //da otkriveno bude odabrani grad
                otkriveno[i]='_';//da otkriveno bude niz __ duzine odabranog grada
            };
        }
        buf=0;//praznjenje buf regisra
        
    }//od whilea

    return (EXIT_SUCCESS);
}