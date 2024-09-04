//
// Bingo 8 bits
//
// Programa realizado por Marcos LM para la reunión de usuarios
// Pixels Retro Party.
//
// Realizado originalmente en Kun Basic.
// Portado a SDCC + Fusion-C
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fusion-c/header/msx_fusion.h"
#include "fusion-c/header/vdp_graph2.h"
#include "fusion-c/header/vars_msxSystem.h"
#include "fusion-c/header/pt3replayer.h"
#include "fusion-c/header/ayfx_player.h"

#define GRIS 		8
#define GRIS_CLARO 12
#define GRIS_OSCURO 5
#define AFB_SIZE 380					// Tamaño del archivo de efectos de sonido para alojar en RAM
#define SFX_BOMBO	1					// Efectos de sonido AYFX
#define SFX_BOLA1	2
#define SFX_BOLA2	3
#define SFX_MENSAJE	4
#define SFX_MENU	5
#define SFX_MENU2	7
#define SFX_SELECT	6

static FCB file;                        // Init the FCB Structure varaible

char tablabola[90][3];					// Array del tablero de bolas (90 bolas -> número bola, x, y)
char paletasc5[64];						// Array para la paleta de color en modo SC5
char bola;								// Última bola sacada
unsigned char filebuf[4532];			// Buffer para cargar imágenes (hasta 17 líneas SC8), también utilizado para backup de zonas de pantalla
unsigned char pt3song[9000];			// Buffer para cargar una música PT3
char s = 0;								// Flag que indica si hay que sacar una bola
char nb = 0;							// Contador de bolas sacadas
char estado = 0;						// Estado del juego:
										// 		0 - Menú nuevo juego
										// 		1 - Menú de juego pausado
										// 		2 - Jugando
										// 		3 - Terminado
										// 		4 - Salir!




/* ---------------------------------
            FT_errorHandler

          In case of Error
-----------------------------------*/ 
void FT_errorHandler(char n, char *name) {
	InitPSG();
	Screen(0);
	SetColors(15,6,6);
	switch (n) {
		case 1:
			printf("\n\rERROR: fcb_open(): %s ",name);
		break;

		case 2:
			printf("\n\rERROR: fcb_close(): %s",name);
		break;  

		case 3:
			printf("\n\rEste programa no funciona en %s",name);
		break; 
	}

	Exit(0);
}



/* ---------------------------------
                FT_SetName

    Set the name of a file to load
                (MSX DOS)
-----------------------------------*/ 
void FT_SetName( FCB *p_fcb, const char *p_name ) {
	char i, j;
	memset( p_fcb, 0, sizeof(FCB) );
	for( i = 0; i < 11; i++ ) {
		p_fcb->name[i] = ' ';
	}
	
	for( i = 0; (i < 8) && (p_name[i] != 0) && (p_name[i] != '.'); i++ ) {
		p_fcb->name[i] =  p_name[i];
	}
	
	if( p_name[i] == '.' ) {
		i++;
		for( j = 0; (j < 3) && (p_name[i + j] != 0) && (p_name[i + j] != '.'); j++ ) {
			p_fcb->ext[j] =  p_name[i + j] ;
		}
	}
}



/* ---------------------------------
          FT_LoadData
  Load Data to a specific pointer
  size is the size of data to read
  skip represent the number of Bytes
  you want to skip from the begining of the file
  Example: skip=7 to skip 7 bytes of a MSX bin
-----------------------------------*/ 
int FT_LoadData(char *file_name, char *buffer, int size, int skip) {   
    FT_SetName( &file, file_name );
    if(fcb_open( &file ) != FCB_SUCCESS) {
		FT_errorHandler(1, file_name);
		return (0);
    }
    
    if (skip>0) {
        fcb_read( &file, buffer, skip );
    }    
    
    fcb_read( &file, buffer, size );
    if( fcb_close( &file ) != FCB_SUCCESS ) {
		FT_errorHandler(2, file_name);
		return (0);
    }

    return(0);
}



/* ---------------------------------
          FT_LoadSc8Image

    Load a SC8 Picture and put datas
  on screen, begining at start_Y line
-----------------------------------*/ 
int FT_LoadSc8Image(char *file_name, unsigned int start_Y, char *buffer) {
	int rd=4352;
	int nbl=0;
	FT_SetName( &file, file_name );
	if(fcb_open( &file ) != FCB_SUCCESS) 	{
		FT_errorHandler(1, file_name);
		return (0);
	}
	fcb_read( &file, buffer, 7 );  // Skip 7 first bytes of the file
	while(rd!=0) {

		rd = fcb_read( &file, buffer, 4352 );
		if (rd!=0) {   
			nbl=rd/256;
			HMMC(buffer, 0,start_Y,256,nbl ); // Move the buffer to VRAM. 17 lines x 256 pixels  from memory
			start_Y=start_Y+nbl; 
		}
	}
	if( fcb_close( &file ) != FCB_SUCCESS ) {
		FT_errorHandler(2, file_name);
		return (0);
	}

	return(1);
}



/* ---------------------------------
          FT_LoadSc5Image

    Load a SC5 Picture and put datas
  on screen, begining at start_Y line
-----------------------------------*/ 
//int FT_LoadSc5Image(char *file_name, unsigned int start_Y, char *buffer) {
int FT_LoadSc5Image(char *file_name, unsigned int start_Y) {

	int rd = 2560;
	int buffer[2560];							// Prefiero utilizar un buffer dentro de la funcíón

	FT_SetName(&file, file_name);
	if(fcb_open(&file) != FCB_SUCCESS) {
		FT_errorHandler(1, file_name);
		return (0);
	}

	fcb_read(&file, buffer, 7);  				// Skip 7 first bytes of the file  
	
	while (rd != 0) {
		MemFill(buffer, 0, 2560);             // Soluciona bug que provoca artefactos en el gráfico
		rd = fcb_read(&file, buffer, 2560);    // Read 20 lines of image data (128bytes per line in screen5)
		HMMC(buffer, 0, start_Y, 256, 20); 	  // Move the buffer to VRAM. 
		start_Y = start_Y + 20;
	}
	
	if( fcb_close(&file) != FCB_SUCCESS) {
		FT_errorHandler(2, file_name);
		return (0);
	}

	return(1);
}



/* -------------------------------------------------------------------------------------
| LOAD BINARY PALETTE. Load a binary image palette (Example Mif UI bin Palette)
| Restrictions      : For screen mode 2,3,4,5,7
| Need              : FT_SetName & FT_errorHandler routines
|
| *file_name        : File name of the palette file to load
| *org_palette      : GLobal 64 bytes array to store the palette (see the SetPalette function)
| Notas: Función propuesta por el gran Eric. No funcionaba correctamente, he tenido que realizar
| alguna modificación. Carga correctamente las paletas de MIFui.
+---------------------------------------------------------------------------------------*/
char FT_LoadBinPalette(char *file_name, char *org_palette) {
	char n, c;
	char byte[7];

	FT_SetName(&file, file_name);
	
	if(fcb_open(&file) != FCB_SUCCESS) {
		FT_errorHandler(1, file_name);
		return (0);
	}     
	n = 0;
	c = 0;
	fcb_read(&file, byte, 7);                             	// Bypass first 7 bytes (Header)
	
	while ( c < 16) {                                       // 16 is the number of colors. Change it to 4 if you want to load a screen 6 Palette
		org_palette[n] = c;
		n++;
		fcb_read(&file, byte, 2);
		org_palette[n] = (byte[0] >> 4) & 0b00000111;         // Red color
		n++;
		org_palette[n] = (byte[1]);                           // green color
		n++;
		org_palette[n] = (byte[0]) & 0b00000111;              // blue color
		n++;
		c++;
	 }

	 if (fcb_close(&file) != FCB_SUCCESS ) {
		FT_errorHandler(2, file_name);
		return (0);
	}
	SetSC5Palette((Palette *)org_palette);                   
return(1);
}



/* ---------------------------------
            FT_CheckFX
     Check if Playing Sound FX
          must be updated
-----------------------------------*/
void FT_CheckFX (void)
{ 
 if (TestFX()==1)
 {
      if (JIFFY!=0)
      {
       JIFFY=0;
        UpdateFX();
      }
  }
}



/* ---------------------------------
            FT_PlayAllFX
       Reproduce un FX de sonido
       completo (hasta el final).
-----------------------------------*/
void FT_PlayAllFX (char sfx) { 
	PlayFX(sfx);
 	while (TestFX() == 1) {
		if (JIFFY != 0) {
		JIFFY = 0;
		UpdateFX();
		}
	}
}



/* ---------------------------------
                FT_Random
           Return a Random Number 
             between a and b-1
-----------------------------------*/ 
char FT_RandomNumber (char a, char b) {
    char random;
    random = rand()%(b - a) + a;  
    return(random);
}



/* ---------------------------------------------------
		Realiza una espera en ciclos
		pasados como parámetro.
-----------------------------------------------------*/ 
void FT_Wait(int ciclos) {
	int i;
	for (i = 0; i < ciclos; i++) Halt();
	FT_CheckFX();										// Motor AYFX
}



/* ---------------------------------------------------
		Muestra la intro del juego
		Dos imágenes mostrándose cíclicamente
		Música de fondo
-----------------------------------------------------*/ 
void FT_Intro() {	
	char cuenta = 0;

	Screen(8);										// Ajustes de modo gráfico y página activa
	SetActivePage(0);
	SetDisplayPage(1);
	SetColors(255, 0, 0);
	Cls();
	PutText(80, 80, "Bingo 8 Bits", 0);
	PutText(78, 92, "Por Marcos LM", 0);
	PutText(25, 116, "Para la Pixels Retro Party", 0);
	PutText(88, 200, "Cargando...", 0);
	SetDisplayPage(0);

	FT_LoadSc8Image("s01.SR8", 256, filebuf);		// Cargamos las imágenes de la intro
	SetDisplayPage(1);
	FT_LoadSc8Image("s02.SR8", 0, filebuf);
	
	InitPSG();										// Cargamos la música e inicializamos el replayer
	FT_LoadData("tn1.pt3", pt3song, 5256, 0);
  	PT3Init (pt3song+99, 0);

  	KillKeyBuffer();
	
	while (Inkey() == 0) {							// Bucle de la intro
		DisableInterupt();
		PT3Rout();
		PT3Play();
		EnableInterupt();
		Halt();
		cuenta++;
		
		if (cuenta == 127) {
			SetDisplayPage(0);
		}
		
		if (cuenta == 255) {
			SetDisplayPage(1);
			cuenta = 0;
		}
	}
	InitPSG();
}



/* ---------------------------------------------------
		Inicializa la tabla de bolas
		y asigna las coordenadas 
		repectivamente en el tablero.
-----------------------------------------------------*/ 
void FT_IniTabla() {
	int y, x, a = 0;

	for (y = 28; y < 191; y = y + 18) {
	
		for (x = 90; x < 235; x = x + 18) {
			tablabola[a][0] = 0;
			tablabola[a][1] = x;
			tablabola[a][2] = y;
			a++;
		}
	}
	tablabola[a][0] = 0;
}



/* ---------------------------------------------------
		Espera a que se suelte la tecla
		si es que hay alguna pulsada.
-----------------------------------------------------*/ 
void FT_WaitKeyRelease () {

	while (JoystickRead(0) != 0) {
		Halt();
	}
	while (TriggerRead(0) != 0) {
		Halt();
	}
	while (Inkey() != 0) {
		Halt();
	}
}



/* ---------------------------------------------------
		Dibuja el recuadro de la "ventana"
		donde se ubica el menú del juego.
-----------------------------------------------------*/ 
void FT_DibujaMenu() {

	BoxFill (70, 40, 186, 130, GRIS, 0 );
	Line (70, 40, 186, 40, GRIS_CLARO, 0);
	Line (70, 40, 70, 130, GRIS_CLARO, 0);
	Line (186, 41, 186, 130, GRIS_OSCURO, 0);
	Line (71, 130, 186, 130, GRIS_OSCURO, 0);
	PlayFX(SFX_MENU2);
}


/* ---------------------------------------------------
		Función principal del menú del juego
		Movemos el cursor entre 3 opciones:
		Continuar: Solo disponible en modo pausa
		Nuevo:     Comenzar un juego nuevo
		Salir:     Salir al DOS
		-----------
		Movimiento: Cursores arriba/abajo
					Selección con espacio
-----------------------------------------------------*/ 
void FT_Menu() {
	int x, y, menu = 1;

	HMCM( 70, 40, 188, 132, filebuf, 0 );			// Salvamos en RAM la zona gráfica donde irá el menú para restaurarla despúes
	FT_DibujaMenu();

	switch (estado) {	
		
		case 0:										// Estado inicial: opción "continuar sombreada"
			LMMM(109, 900, 104, 60, 47, 9, 0);
			for (y = 60; y < 70; y = y + 2) {
				for (x = 104; x < 156; x = x + 2) {
					Pset (x, y, GRIS, 0);
					Pset (x + 1, y + 1, GRIS, 0);
				}
			}
		break;
		
		case 1:										// En pausa
			LMMM(109, 900, 104, 60, 47, 9, 0);
		break;
	}
	LMMM(109, 909, 104, 80, 65, 11, 0);	
	LMMM(156, 900, 104, 100, 23, 9, 0);

	while (TriggerRead(0) != 255) {					// Movimiento entre el menú	
		
		if (JoystickRead(0) == 1) {
			PlayFX(SFX_MENU);
			FT_WaitKeyRelease();
			menu--;
		}
		
		if (JoystickRead(0) == 5) {
			PlayFX(SFX_MENU);
			FT_WaitKeyRelease();
			menu++;
		}
		
		if (menu < 1) menu = 1;						// Comprobamos estado de variable menu
		if (menu < 2 && estado == 0) menu = 2;
		if (menu > 3) menu = 3;
		
		if (menu == 1) {
			LMMM(179 , 900, 90, 59, 7, 12, 0);
			BoxFill (90, 79, 97, 98, GRIS, 0 );
		}
		
		if (menu == 2) {
			LMMM( 179, 900, 90, 79, 7, 12, 0 );
			BoxFill (90, 59, 97, 78, GRIS, 0 );
			BoxFill (90, 99, 97, 118, GRIS, 0 );
		}
		
		if (menu == 3) {
			LMMM(179, 900, 90, 99, 7, 12, 0);
			BoxFill (90, 79, 97, 98, GRIS, 0 );
		}
		FT_CheckFX();								// Motor AYFX
		rand();										// Regeneramos la semilla aleatoria
		Halt();	
	}

	FT_WaitKeyRelease();							// Esperamos a que se suelten las teclas
	FT_PlayAllFX(SFX_SELECT);
	HMMC( filebuf, 70, 40, 118, 92);				// Restauramos el fondo para retirar el menú
	if (menu == 1) estado = 2;						// Continuar (desde juego pausado)
	if (menu == 2) {								// Nuevo Juego
		estado = 2;
		nb = 0;										// Reseteamos el contador de bolas sacadas
		s = 0;										// No hay que sacar nueva bola
		FT_IniTabla();								// Inicializamos la tabla
		HMMM(0, 256, 0, 0, 256, 211);				// Volcamos pantalla sin bolas
	}
	if (menu == 3) estado = 4;						// Salir
	
}



/* ---------------------------------------------------
		Muestra el fotograma pasado de
		la animación del bombo dando
		vueltas.
-----------------------------------------------------*/ 
void FT_AnimBombo(char frame) {	
	switch (frame) {
		
		case 1:
			LMMM(0, 512, 6, 28, 74, 99, 0);
			PlayFX(SFX_BOMBO);
		break;
		
		case 2:
			LMMM(75, 512, 6, 28, 74, 99, 0);
		break;
		
		case 3:
			LMMM(150, 512, 6, 28, 74, 99, 0);
		break;
		
		case 4:
			LMMM(0, 612, 6, 28, 74, 99, 0);
		break;
		
		case 5:
			LMMM(75, 612, 6, 28, 74, 99, 0);
		break;
		
		case 6:
			LMMM(150, 612, 6, 28, 74, 99, 0);
		break;
		
		case 7:
			LMMM(0, 768, 6, 28, 74, 99, 0);
		break;
	}
	FT_Wait(8);
}



/* ---------------------------------------------------
		Realiza una animación de 3 frames
		de la bola saliendo del bombo.
-----------------------------------------------------*/ 
void FT_AnimBola() {
	char frame;

	//PlayFX(SFX_BOLA1);
	FT_PlayAllFX(SFX_BOLA1);
	for (frame = 1; frame < 4; frame++) {
		
		switch (frame) {
			
			case 1:
				LMMM(75, 768, 6, 28, 74, 99, 0);
			break;
			
			case 2:
				LMMM(150, 768, 6, 28, 74, 99, 0);
			break;
			
			case 3:
				LMMM(0, 868, 6, 28, 74, 99, 0);
			break;
		}
		FT_Wait(8);
	}
}



/* ---------------------------------------------------
		Coloca el número de la bola
		en la bola grande.
-----------------------------------------------------*/ 
void FT_PonNumero(char bola) {
	int a, b;

	LMMM(25, 413, 25, 157, 38, 25, 0);						// Eliminamos número anterior
	
	if (bola < 10) {										// Número de una cifra
		LMMM(75 + bola * 18, 868, 35, 157, 18, 25, 0);
	}
	if (bola > 9) {											// Número de dos cifras
		a = bola / 10;
		b = bola - a * 10;
		LMMM(75 + a * 18, 868, 25, 157, 18, 25, 0);
		LMMM(75 + b * 18, 868, 44, 157, 18, 25, 0);
	}
}



/* ---------------------------------------------------
		Coloca la bola pequeña en la 
		correspondiente posición del
		tablero,
-----------------------------------------------------*/ 
void FT_ColocaBola(char bola) {
	int a, b;

	LMMM(75, 900, tablabola[bola-1][1], tablabola[bola-1][2], 16, 16, 0);						// Copiamos gráfico bola sin número
	if (bola < 10) {																			// Números de una sola cifra
		LMMM(75 + bola * 6, 893, tablabola[bola-1][1] + 5, tablabola[bola-1][2] + 5, 6, 7, 0 );	
	}
	if (bola > 9) {																				// Números de dos cifras
		a = bola / 10;
		b = bola - a * 10;
		LMMM(75 + a * 6, 893, tablabola[bola-1][1] + 2, tablabola[bola-1][2] + 5, 6, 7, 0 );
		LMMM(75 + b * 6, 893, tablabola[bola-1][1] + 8, tablabola[bola-1][2] + 5, 6, 7, 0 );
	}
}







void main(void) {

	char frame = 1;

	if (ReadMSXtype() == 0) FT_errorHandler(3, "MSX 1");	// Solo para MSX2 o superior, sorry
	KeySound(0);											// Deshabilita el sonido "click" de las teclas
	afbdata = MMalloc(AFB_SIZE);							// Cargamos e inicializamos los FX de sonido
	InitPSG();
	InitFX();
	FT_LoadData("fx.afb", afbdata, AFB_SIZE, 0);
	FT_Intro();												// Lanzamos la intro

	Screen(5);												// Ajustes del modo gráfico del juego
	SetActivePage(0);
	SetDisplayPage(0);
	FT_LoadBinPalette("G00.Pl5", paletasc5);				// Cargamos la paleta
	SetColors(15, 0, 0);
	PutText(85, 92, "Cargando...", 0);
	FT_LoadSc5Image("G00.SC5", 256);						// Cargamos los gráficos
	FT_LoadSc5Image("G01.SC5", 512);
	FT_LoadSc5Image("G02.SC5", 768);
	SetActivePage(1);
	SetColors(2, 12, 0);
	PutText(124, 8, "Pixels Party Ed.", 0);
	HMMM(0, 256, 0, 0, 256, 211);
	SetActivePage(0);
	

	while (1) {												// Bucle principal del juego ---------------------------------------
		switch(estado){
			case 0:											// Menú Nuevo Juego
				FT_Menu();
			break;

			case 1:											// Menú Pausa
				FT_Menu();
			break;

			case 2:											// Jugando
				
				if (nb < 90) {
					FT_AnimBombo(frame);					// Animación del bombo
					frame++;
					if (frame==8) frame=1;
				}
				
				if (s == 1 && frame == 7) {					// El flag de sacar bola está activado y la animación del bombo está al final
					nb++;									// Incrementamos contador de bolas sacadas
					
					if (nb < 91) {							// Comprobamos si ya están todas las bolas fuera
						FT_AnimBola();
						
						do {								// Sorteamos!
							bola=FT_RandomNumber(1, 91);
						} while (tablabola[bola][0] == 1);
						
						tablabola[bola][0] = 1;				// Asignamos bola
						s = 0;								// Desactivamos flag de sacar bola
						FT_PonNumero(bola);					// Ponemos número en la bola grande
						FT_ColocaBola(bola);				// Colocamos la bola en el tablero
					}
					else {
						estado = 3;							// Fin del juego
					}
				}
			break;

			case 3:											// Fin del juego
				FT_DibujaMenu();
				PutText (110, 50, "JUEGO", 1);
				PutText (88, 60, "FINALIZADO", 1);
				PutText (92, 94, "Pulsa una", 1);
				PutText (108, 106, "tecla", 1);
				FT_PlayAllFX(SFX_MENSAJE);
				KillKeyBuffer();
				WaitKey();
				estado = 0;
			break;

			case 4:											// Salir del juego
				RestoreSC5Palette();
				SetColors(15, 4, 4);
				Screen(0);
				Cls();
				printf ("Espero que hayas disfrutado!\n\r\n");
				FreeFX();									// Liberamos memoria usada por AYFX
				KillKeyBuffer();
				InitPSG();
				Exit(0);
			break;
		}

		FT_CheckFX();										// Motor AYFX
		if (Inkey() == 27 && estado == 2) {					// La tecla Esc lanza el menú de pausa
			FT_WaitKeyRelease();
			estado = 1;
		}

		if (TriggerRead(0)==255 && estado==2) {				// Sacar bola!
			//FT_WaitKeyRelease();
			s = 1;
		}

	}

}
 