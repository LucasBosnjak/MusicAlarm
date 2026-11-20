#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>
#include <conio.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

#define MAX_HERATYKSET 15
#define PUSKURI_KOKO 100
#define TALLENNUSTIEDOSTO "heratykset.txt"

typedef struct {
    int tunnit;
    int minuutit;
    bool aktiivinen;
} Heratys;

Heratys heratykset[MAX_HERATYKSET];
int maara = 0;

// MP3-tiedostojen nimet (samaan kansioon .exe-tiedoston kanssa)
const char *musiikit[3] = {
    "alarm1.mp3",
    "alarm2.mp3",
    "alarm3.mp3"
};
int valittu_musiikki = 0;

// --- TIEDOSTONKÄSITTELY ---

void tallenna_tiedostoon() {
    FILE *tiedosto = fopen(TALLENNUSTIEDOSTO, "w");
    if (tiedosto == NULL) {
        printf("Virhe: Tiedostoon '%s' kirjoittaminen epäonnistui!\n", TALLENNUSTIEDOSTO);
        return;
    }

    // 1. rivi: valittu musiikki (0-2) ja herätysten määrä
    fprintf(tiedosto, "%d %d\n", valittu_musiikki, maara);

    // Seuraavat rivit: jokaisen herätyksen tunnit, minuutit ja tila (1 tai 0)
    for (int i = 0; i < maara; i++) {
        fprintf(tiedosto, "%d %d %d\n",
                heratykset[i].tunnit,
                heratykset[i].minuutit,
                heratykset[i].aktiivinen ? 1 : 0);
    }

    fclose(tiedosto);
}

void lataa_tiedostosta() {
    FILE *tiedosto = fopen(TALLENNUSTIEDOSTO, "r");
    if (tiedosto == NULL) {
        // Tiedostoa ei ole vielä tai se on tyhjä
        return;
    }

    int ladattu_musiikki, ladattu_maara;
    if (fscanf(tiedosto, "%d %d", &ladattu_musiikki, &ladattu_maara) == 2) {
        if (ladattu_musiikki >= 0 && ladattu_musiikki < 3) {
            valittu_musiikki = ladattu_musiikki;
        }

        maara = 0;
        for (int i = 0; i < ladattu_maara && i < MAX_HERATYKSET; i++) {
            int h, m, akt;
            if (fscanf(tiedosto, "%d %d %d", &h, &m, &akt) == 3) {
                if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
                    heratykset[maara] = (Heratys){
                        .tunnit = h,
                        .minuutit = m,
                        .aktiivinen = (akt == 1)
                    };
                    maara++;
                }
            }
        }
    }

    fclose(tiedosto);
}

// --- APUFUNKTIOT ---

void tyhjenna_syotepuskuri() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void tulosta_ohje_ja_heratykset() {
    printf("\n=== HERÄTYSKELLO ===\n");
    printf("Komennot:\n");
    printf("  new alarm    - Lisää uusi herätys\n");
    printf("  adjust       - Muokkaa olemassa olevaa herätystä\n");
    printf("  activate     - Laita herätys päälle tai pois\n");
    printf("  choose music - Valitse herätysääni (1-3)\n");
    printf("  empty        - Tyhjennä kaikki herätykset\n");
    printf("  help         - Näytä tämä valikko ja herätykset\n");
    printf("  exit         - Sulje ohjelma\n");
    printf("  seuraavaan         - aikaa seuraavaan herätykseen\n");

    printf("\nValittu herätysääni: %d. %s\n", valittu_musiikki + 1, musiikit[valittu_musiikki]);
    printf("\n--- Tallennetut hälytykset (%d/%d) ---\n", maara, MAX_HERATYKSET);

    if (maara == 0) {
        printf("  Ei tallennettuja hälytyksiä.\n");
    } else {
        for (int i = 0; i < maara; i++) {
            printf("  %d. %02d:%02d [%s]\n",
                   i + 1,
                   heratykset[i].tunnit,
                   heratykset[i].minuutit,
                   heratykset[i].aktiivinen ? "AKTIIVINEN" : "POIS");
        }
    }
    printf("------------------------------------\n\n");
}


void seuraavaan() {
    if (maara == 0) {
        printf("Virhe: Herätysmuisti tyhjä!\n");
        return;
    }

    time_t nyt;
    time(&nyt);
    struct tm *paikallinen = localtime(&nyt);
    
    // Muutetaan nykyhetki minuuteiksi (esim. 10:30 -> 630 min)
    int nyt_minuutit = paikallinen->tm_hour * 60 + paikallinen->tm_min;

    int pienin_ero = 24 * 60 + 1; // Suurempi kuin vuorokauden minuutit (1440)
    int index = -1;

    for (int i = 0; i < maara; i++) {
        if (!(heratykset[i].aktiivinen)) {
            continue;
        }

        int heratys_minuutit = heratykset[i].tunnit * 60 + heratykset[i].minuutit;
        int ero = heratys_minuutit - nyt_minuutit;

        // Jos herätysaika on jo mennyt tänään (tai on juuri nyt), se soi huomenna (+ 24h)
        if (ero <= 0) {
            ero += 24 * 60;
        }

        if (ero < pienin_ero) {
            pienin_ero = ero;
            index = i;
        }
    }

    if (index == -1) {
        printf("Virhe: Ei aktiivisia herätyksiä!\n");
        return;
    }

    // Muutetaan pienin minuuttiero takaisin tunneiksi ja minuuteiksi
    int lahin_h = pienin_ero / 60;
    int lahin_min = pienin_ero % 60;

    printf("Lähimpään herätykseen (%02d:%02d) aikaa: %d h %d min!\n",
           heratykset[index].tunnit,
           heratykset[index].minuutit,
           lahin_h,
           lahin_min);
}


void lisaa_heratys_komento() {
    if (maara >= MAX_HERATYKSET) {
        printf("Virhe: Herätysmuisti on täynnä (max %d)!\n", MAX_HERATYKSET);
        return;
    }

    int h, m, akt;
    printf("Anna tunnit (0-23), minuutit (0-59) ja tila (1 = aktiivinen, 0 = pois), esim. '7 30 1': ");

    if (scanf("%d %d %d", &h, &m, &akt) != 3) {
        printf("Virhe: Virheellinen syöte! Anna kolme kokonaislukua.\n");
        tyhjenna_syotepuskuri();
        return;
    }
    tyhjenna_syotepuskuri();

    if (h < 0 || h > 23 || m < 0 || m > 59 || (akt != 0 && akt != 1)) {
        printf("Virhe: Arvot eivät ole sallituissa rajoissa (tunnit 0-23, minuutit 0-59, tila 0 tai 1).\n");
        return;
    }

    heratykset[maara] = (Heratys){ .tunnit = h, .minuutit = m, .aktiivinen = (akt == 1) };
    maara++;
    
    tallenna_tiedostoon();
    printf("OK: Herätys %02d:%02d lisätty (%s) ja tallennettu.\n", h, m, akt == 1 ? "aktiivinen" : "pois päältä");
}

void muokkaa_heratysta_komento() {
    if (maara == 0) {
        printf("Virhe: Ei muokattavia herätyksiä.\n");
        return;
    }

    int nro, h, m;
    printf("Anna muokattavan herätyksen numero (1-%d): ", maara);
    if (scanf("%d", &nro) != 1 || nro < 1 || nro > maara) {
        printf("Virhe: Virheellinen herätyksen numero.\n");
        tyhjenna_syotepuskuri();
        return;
    }

    printf("Anna uudet tunnit (0-23) ja minuutit (0-59): ");
    if (scanf("%d %d", &h, &m) != 2 || h < 0 || h > 23 || m < 0 || m > 59) {
        printf("Virhe: Virheellinen kellonaika.\n");
        tyhjenna_syotepuskuri();
        return;
    }
    tyhjenna_syotepuskuri();

    heratykset[nro - 1].tunnit = h;
    heratykset[nro - 1].minuutit = m;
    
    tallenna_tiedostoon();
    printf("OK: Herätys %d päivitetty aikaan %02d:%02d ja tallennettu.\n", nro, h, m);
}

void aktivoi_heratys_komento() {
    if (maara == 0) {
        printf("Virhe: Ei herätyksiä listalla.\n");
        return;
    }

    int nro, akt;
    printf("Anna herätyksen numero (1-%d) ja uusi tila (1 = päälle, 0 = pois): ", maara);
    if (scanf("%d %d", &nro, &akt) != 2 || nro < 1 || nro > maara || (akt != 0 && akt != 1)) {
        printf("Virhe: Virheellinen numero tai tila.\n");
        tyhjenna_syotepuskuri();
        return;
    }
    tyhjenna_syotepuskuri();

    heratykset[nro - 1].aktiivinen = (akt == 1);
    
    tallenna_tiedostoon();
    printf("OK: Herätys %d (%02d:%02d) on nyt %s.\n",
           nro, heratykset[nro - 1].tunnit, heratykset[nro - 1].minuutit,
           akt == 1 ? "AKTIIVINEN" : "POIS PÄÄLTÄ");
}

void valitse_musiikki_komento() {
    printf("\nValitse herätysmusiikki:\n");
    for (int i = 0; i < 3; i++) {
        printf("  %d. %s %s\n", i + 1, musiikit[i], (i == valittu_musiikki) ? "(nykyinen)" : "");
    }
    printf("Valinta (1, 2 tai 3): ");

    int valinta;
    if (scanf("%d", &valinta) != 1 || valinta < 1 || valinta > 3) {
        printf("Virhe: Valitse numero 1, 2 tai 3.\n");
        tyhjenna_syotepuskuri();
        return;
    }
    tyhjenna_syotepuskuri();

    valittu_musiikki = valinta - 1;
    
    tallenna_tiedostoon();
    printf("OK: Herätysääneksi valittu %s.\n", musiikit[valittu_musiikki]);
}

void tyhjenna_muisti_komento() {
    maara = 0;
    tallenna_tiedostoon();
    printf("OK: Kaikki herätykset tyhjennetty muistista ja tiedostosta.\n");
}

// --- HÄLYTYKSEN SOITTO JA DEAKTIVOINTI ---

void laukaise_halytys(int indeksi) {
    char mci_komento[256];

    snprintf(mci_komento, sizeof(mci_komento), "open \"%s\" type mpegvideo alias alarm_sound", musiikit[valittu_musiikki]);
    mciSendStringA(mci_komento, NULL, 0, NULL);
    mciSendStringA("play alarm_sound repeat", NULL, 0, NULL);

    printf("\n\n=============================================\n");
    printf("HÄLYTYS %d (%02d:%02d)! Paina SPACEBAR lopettaaksesi!\n",
           indeksi + 1, heratykset[indeksi].tunnit, heratykset[indeksi].minuutit);
    printf("=============================================\n");

    while (true) {
        printf("ppprrrrrr prrrrrr\n");

        for (int i = 0; i < 10; i++) {
            if (_kbhit()) {
                int nappain = _getch();
                if (nappain == ' ') {
                    mciSendStringA("stop alarm_sound", NULL, 0, NULL);
                    mciSendStringA("close alarm_sound", NULL, 0, NULL);
                    
                    heratykset[indeksi].aktiivinen = false;
                    tallenna_tiedostoon(); // Tallennetaan tieto, että hälytys on nyt pois päältä
                    
                    printf("\nHälytys deaktivoitu! Musiikki pysäytetty.\n> ");
                    return;
                }
            }
            Sleep(50);
        }
    }
}

void kasittele_komento(const char *komento) {
    if (strcmp(komento, "new alarm") == 0) {
        lisaa_heratys_komento();
    } else if (strcmp(komento, "adjust") == 0) {
        muokkaa_heratysta_komento();
    } else if (strcmp(komento, "activate") == 0) {
        aktivoi_heratys_komento();
    } else if (strcmp(komento, "choose music") == 0) {
        valitse_musiikki_komento();
    } else if (strcmp(komento, "empty") == 0) {
        tyhjenna_muisti_komento();
    } else if (strcmp(komento, "help") == 0) {
        tulosta_ohje_ja_heratykset();
    }  else if (strcmp(komento, "seuraavaan") == 0) {
        seuraavaan();
    } else if (strcmp(komento, "exit") == 0) {
        printf("Suljetaan herätyskello...\n");
        exit(0);
    } else if (strlen(komento) > 0) {
        printf("Tuntematon komento: '%s'. Kirjoita 'help' nähdäksesi komennot.\n", komento);
    }
}

// --- PÄÄOHJELMA ---

int main() {
    SetConsoleOutputCP(65001);

    // Ladataan aiemmin tallennetut herätykset ja musiikkivalinta tiedostosta
    lataa_tiedostosta();

    char syote[PUSKURI_KOKO];
    int syote_indeksi = 0;
    int viimeksi_tarkistettu_minuutti = -1;

    tulosta_ohje_ja_heratykset();
    printf("> ");

    while (true) {
        // 1. TARKISTETAAN KELLONAIKA
        time_t nyt;
        time(&nyt);
        struct tm *paikallinen = localtime(&nyt);

        if (paikallinen->tm_min != viimeksi_tarkistettu_minuutti) {
            viimeksi_tarkistettu_minuutti = paikallinen->tm_min;

            for (int i = 0; i < maara; i++) {
                if (heratykset[i].aktiivinen &&
                    heratykset[i].tunnit == paikallinen->tm_hour &&
                    heratykset[i].minuutit == paikallinen->tm_min) {

                    syote_indeksi = 0;
                    laukaise_halytys(i);
                }
            }
        }

        // 2. LUETAAN NÄPPÄIMISTÖPUSKURI TYHJÄKSI ILMAN VIIVETTÄ
        while (_kbhit()) {
            int ch = _getch();

            if (ch == '\r' || ch == '\n') {
                printf("\n");
                syote[syote_indeksi] = '\0';
                kasittele_komento(syote);
                syote_indeksi = 0;
                printf("> ");
            } else if (ch == '\b' || ch == 127) {
                if (syote_indeksi > 0) {
                    syote_indeksi--;
                    printf("\b \b");
                }
            } else if (syote_indeksi < PUSKURI_KOKO - 1 && ch >= 32 && ch <= 126) {
                syote[syote_indeksi++] = (char)ch;
                putchar(ch);
            }
        }

        Sleep(50);
    }

    return 0;
}