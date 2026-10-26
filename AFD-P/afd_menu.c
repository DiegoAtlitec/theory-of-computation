// gcc afd_menu.c -o afd -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE -L/opt/homebrew/lib -lSDL2 -lSDL2_ttf

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_STATES  32
#define MAX_SYMBOLS 8
#define MAX_STR     256

typedef struct {
    char symbols[MAX_SYMBOLS];          
    int  numSymbols;                    
    char states[MAX_STATES][16];        
    int  numStates;                     
    char trans[MAX_STATES][MAX_SYMBOLS][16]; 
    char start[16];                     
    char accept[MAX_STATES][16];        
    int  numAccept;                     
} AFD;

typedef struct {
    SDL_Rect rect;
    SDL_Color color;
    char label[64];
} Button;

typedef enum {
    SCREEN_MENU = 0,
    SCREEN_SIM
} Screen;

// ──────────────────────────────────────────────
// UTILIDADES DE DIBUJO
// ──────────────────────────────────────────────
void drawCircle(SDL_Renderer *ren, int cx, int cy, int radius) {
    for (int w = -radius; w <= radius; w++) {
        for (int h = -radius; h <= radius; h++) {
            if (w*w + h*h <= radius*radius) SDL_RenderDrawPoint(ren, cx + w, cy + h);
        }
    }
}

void drawRoundedRect(SDL_Renderer *ren, SDL_Rect rect, SDL_Color color, int radius) {
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, 255);
    SDL_Rect center = {rect.x + radius, rect.y, rect.w - 2*radius, rect.h};
    SDL_RenderFillRect(ren, &center);
    SDL_Rect middle = {rect.x, rect.y + radius, rect.w, rect.h - 2*radius};
    SDL_RenderFillRect(ren, &middle);
    drawCircle(ren, rect.x + radius, rect.y + radius, radius);
    drawCircle(ren, rect.x + rect.w - radius - 1, rect.y + radius, radius);
    drawCircle(ren, rect.x + radius, rect.y + rect.h - radius - 1, radius);
    drawCircle(ren, rect.x + rect.w - radius - 1, rect.y + rect.h - radius - 1, radius);
}

void drawText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!text || !*text) return;
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect rect = {x, y, surf->w, surf->h};
    SDL_RenderCopy(renderer, tex, NULL, &rect);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

int pointInRect(int x, int y, SDL_Rect *r) {
    return (x > r->x && x < r->x + r->w && y > r->y && y < r->y + r->h);
}

// ──────────────────────────────────────────────
// LÓGICA DEL AFD
// ──────────────────────────────────────────────
static int is_accept_state(AFD *afd, const char *stateName) {
    for (int i = 0; i < afd->numAccept; i++)
        if (strcmp(afd->accept[i], stateName) == 0) return 1;
    return 0;
}

static int state_index(AFD *afd, const char *name) {
    for (int i = 0; i < afd->numStates; i++)
        if (strcmp(afd->states[i], name) == 0) return i;
    return -1;
}

static int symbol_index(AFD *afd, char sym) {
    for (int i = 0; i < afd->numSymbols; i++)
        if (afd->symbols[i] == sym) return i;
    return -1;
}

static void trim(char *s) {
    if (!s) return;
    // left
    int i = 0; while (isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s+i, strlen(s+i)+1);
    // right
    int n = (int)strlen(s); 
    while (n>0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}

// Carga AFD desde archivo:
//  - 1ª línea: símbolos separados por espacio (p.ej. "a b c" o "0 1")
//  - Resto: "estado  next(symbol0)  next(symbol1)  ..."
static int loadAFD_from_file(AFD *afd, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { printf("No se puede abrir %s\n", filename); return 0; }

    memset(afd, 0, sizeof(*afd));

    // Leer primera línea de símbolos
    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    trim(line);
    afd->numSymbols = 0;
    {
        char *tok = strtok(line, " \t\r\n");
        while (tok && afd->numSymbols < MAX_SYMBOLS) {
            afd->symbols[afd->numSymbols++] = tok[0];
            tok = strtok(NULL, " \t\r\n");
        }
    }
    if (afd->numSymbols <= 0) { fclose(f); return 0; }

    // Leer lineas de estados + transiciones
    int sCount = 0;
    while (fgets(line, sizeof(line), f) && sCount < MAX_STATES) {
        trim(line);
        if (!*line) continue;

        // Tokenizar
        char *tok = strtok(line, " \t\r\n");
        if (!tok) continue;
        strncpy(afd->states[sCount], tok, sizeof(afd->states[sCount])-1);
        afd->states[sCount][sizeof(afd->states[sCount])-1] = '\0';

        for (int j = 0; j < afd->numSymbols; j++) {
            tok = strtok(NULL, " \t\r\n");
            if (!tok) { fclose(f); return 0; }
            strncpy(afd->trans[sCount][j], tok, sizeof(afd->trans[sCount][j])-1);
            afd->trans[sCount][j][sizeof(afd->trans[sCount][j])-1] = '\0';
        }
        sCount++;
    }
    afd->numStates = sCount;

    // Estado inicial por defecto: q0
    strcpy(afd->start, "q0");
    fclose(f);
    return 1;
}

// Configura los estados de aceptación según AFD 
static void set_accepting_by_problem(AFD *afd, int which) {
    afd->numAccept = 0;
    if (which == 1) {
        // AFD 1: acepta si termina en q4
        strcpy(afd->accept[afd->numAccept++], "q4");
    } else if (which == 2) {
        // AFD 2: ->*q0, *q1, *q3
        strcpy(afd->accept[afd->numAccept++], "q0");
        strcpy(afd->accept[afd->numAccept++], "q1");
        strcpy(afd->accept[afd->numAccept++], "q3");
    } else if (which == 3) {
        // AFD 3: ->*q0 y *q3
        strcpy(afd->accept[afd->numAccept++], "q0");
        strcpy(afd->accept[afd->numAccept++], "q3");
    }
}

int simulateAFD(AFD *afd, const char *input, char trace[MAX_STR][64], int *steps, int path[MAX_STR]) {
    char current[16];
    strcpy(current, afd->start);
    *steps = 0;

    // Registrar estado inicial en path
    int sidx = state_index(afd, current);
    if (sidx >= 0) path[*steps] = sidx;

    for (int i = 0; input[i]; i++) {
        sidx = state_index(afd, current);
        int symi = symbol_index(afd, input[i]);
        if (sidx == -1 || symi == -1) return 0;

        snprintf(trace[*steps], 63, "%s --%c--> %s", current, input[i], afd->trans[sidx][symi]);
        trace[*steps][63] = '\0';
        (*steps)++;
        strcpy(current, afd->trans[sidx][symi]);
        path[*steps] = state_index(afd, current);
    }

    return is_accept_state(afd, current);
}

// ──────────────────────────────────────────────
// INTERFAZ
// ──────────────────────────────────────────────
void drawButton(SDL_Renderer *ren, TTF_Font *font, Button *btn, int hovered) {
    SDL_Color bg = hovered ? (SDL_Color){191, 199, 209} : btn->color;
    drawRoundedRect(ren, btn->rect, bg, 10);
    SDL_Color white = {255, 255, 255};
    drawText(ren, font, btn->label, btn->rect.x + 20, btn->rect.y + 7, white);
}

void drawTransitionTable(SDL_Renderer *ren, TTF_Font *font, AFD *afd, int highlightIdx) {
    int startY = 220;
    int startX = 500;
    SDL_Color white = {255,255,255};
    SDL_Color yellow = {255,255,0};
    char buf[256];

    drawText(ren, font, "Tabla de transiciones:", startX, startY - 30, white);

    // Encabezado de símbolos
    buf[0] = '\0';
    strcat(buf, "      ");
    for (int j = 0; j < afd->numSymbols; j++) {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "   %c", afd->symbols[j]);
        strcat(buf, tmp);
    }
    drawText(ren, font, buf, startX + 20, startY, white);
    startY += 30;

    // Filas de estados
    for (int i = 0; i < afd->numStates; i++) {
        SDL_Color color = (i == highlightIdx) ? yellow : white;
        char row[256];
        snprintf(row, sizeof(row), "%3s", afd->states[i]);
        for (int j = 0; j < afd->numSymbols; j++) {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "  %5s", afd->trans[i][j]);
            strncat(row, tmp, sizeof(row)-strlen(row)-1);
        }
        drawText(ren, font, row, startX, startY + i * 25, color);
    }
}

static int char_is_in_symbols(AFD *afd, char c) {
    return symbol_index(afd, c) != -1;
}

// ──────────────────────────────────────────────
// CARGA DE FUENTE CON FALLBACKS
// ──────────────────────────────────────────────
static TTF_Font* load_font_or_fallback(int ptsize) {
    // 1) Archivo local
    TTF_Font *f = TTF_OpenFont("Arial.ttf", ptsize);
    if (f) return f;
#ifdef _WIN32
    f = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", ptsize);
    if (f) return f;
#endif
#ifdef __APPLE__
    f = TTF_OpenFont("/System/Library/Fonts/Supplemental/Arial.ttf", ptsize);
    if (f) return f;
#endif
    // Linux comunes
    f = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", ptsize);
    if (f) return f;

    // Último intento: NULL
    return NULL;
}

// ──────────────────────────────────────────────
// MAIN
// ──────────────────────────────────────────────
int main(int argc, char **argv) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) { printf("SDL Init error: %s\n", SDL_GetError()); return 1; }
    if (TTF_Init() != 0) { printf("TTF Init error: %s\n", TTF_GetError()); SDL_Quit(); return 1; }

    SDL_Window *win = SDL_CreateWindow("Simulador de AFD TC - 2025",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 680, 0);
    if (!win) { printf("Window error: %s\n", SDL_GetError()); return 1; }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { printf("Renderer error: %s\n", SDL_GetError()); return 1; }

    TTF_Font *font = load_font_or_fallback(22);
    if (!font) { printf("Error cargando fuente: %s\n", TTF_GetError()); return 1; }

    // Estados de UI
    Screen screen = SCREEN_MENU;

    // Botones de menu
    Button btnAFD1 = {{340, 180, 220, 50}, {70, 130, 200}, "AFD 1"};
    Button btnAFD2 = {{340, 250, 220, 50}, {155, 228, 76}, "AFD 2"};
    Button btnAFD3 = {{340, 320, 220, 50}, {230, 46, 46}, "AFD 3"};

    // Botón volver
    Button btnBack = {{700, 20, 175, 40}, {120, 120, 120}, "Volver al menu"};

    // Controles del simulador
    SDL_Rect inputBox = {50, 80, 400, 40};
    Button btnTest = {{470, 80, 200, 40}, {70, 130, 200}, "Probar cadena"};

    // Cursores
    SDL_Cursor *arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    SDL_Cursor *ibeam = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    SDL_Cursor *hand  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

    // Datos del AFD y simulación
    AFD afd;
    int currentAFD = 0; // 1, 2, 3
    char currentFile[64] = "";

    char input[MAX_STR] = "";
    int cursorPos = 0, focused = 0;
    int accepted = -1, steps = 0, highlight = -1, path[MAX_STR];
    char trace[MAX_STR][64];
    int scrollY = 0, scrollX = 0;

    Uint32 lastTime = 0;
    int animStep = 0;

    SDL_StartTextInput();

    int running = 1;
    while (running) {
        SDL_Event e;
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        int hoverBack=0, hoverTest=0, hoverInput=0, hover1=0, hover2=0, hover3=0;

        if (screen == SCREEN_MENU) {
            hover1 = pointInRect(mx, my, &btnAFD1.rect);
            hover2 = pointInRect(mx, my, &btnAFD2.rect);
            hover3 = pointInRect(mx, my, &btnAFD3.rect);
            if (hover1 || hover2 || hover3) SDL_SetCursor(hand);
            else SDL_SetCursor(arrow);
        } else {
            hoverBack  = pointInRect(mx, my, &btnBack.rect);
            hoverTest  = pointInRect(mx, my, &btnTest.rect);
            hoverInput = pointInRect(mx, my, &inputBox);
            if (hoverBack || hoverTest) SDL_SetCursor(hand);
            else if (hoverInput || focused) SDL_SetCursor(ibeam);
            else SDL_SetCursor(arrow);
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = 0; }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (screen == SCREEN_MENU) {
                    if (pointInRect(e.button.x, e.button.y, &btnAFD1.rect)) {
                        currentAFD = 1; strcpy(currentFile, "afd1.txt");
                        if (!loadAFD_from_file(&afd, currentFile)) { printf("Error cargando %s\n", currentFile); continue; }
                        set_accepting_by_problem(&afd, currentAFD);
                        // reset simulación
                        input[0]='\0'; cursorPos=0; focused=0;
                        accepted=-1; steps=0; highlight=-1; scrollY=0; scrollX=0; animStep=0;
                        screen = SCREEN_SIM;
                    } else if (pointInRect(e.button.x, e.button.y, &btnAFD2.rect)) {
                        currentAFD = 2; strcpy(currentFile, "afd2.txt");
                        if (!loadAFD_from_file(&afd, currentFile)) { printf("Error cargando %s\n", currentFile); continue; }
                        set_accepting_by_problem(&afd, currentAFD);
                        input[0]='\0'; cursorPos=0; focused=0;
                        accepted=-1; steps=0; highlight=-1; scrollY=0; scrollX=0; animStep=0;
                        screen = SCREEN_SIM;
                    } else if (pointInRect(e.button.x, e.button.y, &btnAFD3.rect)) {
                        currentAFD = 3; strcpy(currentFile, "afd3.txt");
                        if (!loadAFD_from_file(&afd, currentFile)) { printf("Error cargando %s\n", currentFile); continue; }
                        set_accepting_by_problem(&afd, currentAFD);
                        input[0]='\0'; cursorPos=0; focused=0;
                        accepted=-1; steps=0; highlight=-1; scrollY=0; scrollX=0; animStep=0;
                        screen = SCREEN_SIM;
                    }
                } else { // SIM
                    focused = pointInRect(e.button.x, e.button.y, &inputBox);
                    if (pointInRect(e.button.x, e.button.y, &btnTest.rect)) {
                        accepted = simulateAFD(&afd, input, trace, &steps, path);
                        highlight = (steps>=0) ? path[0] : -1;
                        scrollY = 0; animStep = 0; lastTime = SDL_GetTicks();
                    } else if (pointInRect(e.button.x, e.button.y, &btnBack.rect)) {
                        screen = SCREEN_MENU;
                    }
                }
            }
            else if (e.type == SDL_MOUSEWHEEL && screen == SCREEN_SIM) {
                scrollY -= e.wheel.y * 30;
                if (scrollY < 0) scrollY = 0;
                int maxScroll = steps * 30 - 300;
                if (maxScroll < 0) maxScroll = 0;
                if (scrollY > maxScroll) scrollY = maxScroll;
            }
            else if (e.type == SDL_TEXTINPUT && screen == SCREEN_SIM && focused) {
                char c = e.text.text[0];
                if (char_is_in_symbols(&afd, c)) {
                    if ((int)strlen(input) < MAX_STR - 1) {
                        memmove(&input[cursorPos + 1], &input[cursorPos], strlen(input) - cursorPos + 1);
                        input[cursorPos] = c;
                        cursorPos++;
                    }
                }
            }
            else if (e.type == SDL_KEYDOWN && screen == SCREEN_SIM && focused) {
                if (e.key.keysym.sym == SDLK_LEFT && cursorPos > 0) cursorPos--;
                else if (e.key.keysym.sym == SDLK_RIGHT && cursorPos < (int)strlen(input)) cursorPos++;
                else if (e.key.keysym.sym == SDLK_BACKSPACE && cursorPos > 0) {
                    memmove(&input[cursorPos - 1], &input[cursorPos], strlen(input) - cursorPos + 1);
                    cursorPos--;
                } else if (e.key.keysym.sym == SDLK_DELETE && cursorPos < (int)strlen(input)) {
                    memmove(&input[cursorPos], &input[cursorPos + 1], strlen(input) - cursorPos);
                } else if (e.key.keysym.sym == SDLK_RETURN) {
                    accepted = simulateAFD(&afd, input, trace, &steps, path);
                    highlight = (steps>=0) ? path[0] : -1;
                    scrollY = 0; animStep = 0; lastTime = SDL_GetTicks();
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    screen = SCREEN_MENU;
                }
            }
        }

        // Animación de resaltado cada 500 ms
        if (screen == SCREEN_SIM && accepted != -1 && SDL_GetTicks() - lastTime > 500 && animStep <= steps) {
            highlight = path[animStep];
            animStep++;
            lastTime = SDL_GetTicks();
        }

        // Scroll horizontal del input (para cursor visible)
        if (screen == SCREEN_SIM) {
            int totalW = 0, cursorX = 0;
            char temp[MAX_STR];
            TTF_SizeText(font, input, &totalW, NULL);
            if (cursorPos > 0) {
                strncpy(temp, input, cursorPos);
                temp[cursorPos] = '\0';
                TTF_SizeText(font, temp, &cursorX, NULL);
            }
            int visibleWidth = inputBox.w - 20;
            if (cursorX - scrollX > visibleWidth) scrollX = cursorX - visibleWidth + 10;
            else if (cursorX < scrollX) scrollX = cursorX - 10;
            if (scrollX < 0) scrollX = 0;
        }

        // ───────── Render ─────────
        SDL_SetRenderDrawColor(ren, 25, 25, 25, 255);
        SDL_RenderClear(ren);
        SDL_Color white = {255,255,255};
        SDL_Color inputBg = {50,50,50};

        if (screen == SCREEN_MENU) {
            drawText(ren, font, "Seleccione un AFD", 340, 110, white);
            drawButton(ren, font, &btnAFD1, hover1);
            drawButton(ren, font, &btnAFD2, hover2);
            drawButton(ren, font, &btnAFD3, hover3);

            drawText(ren, font, "Archivos esperados en el directorio:", 240, 420, white);
            drawText(ren, font, "afd1.txt, afd2.txt, afd3.txt", 300, 450, white);
        } else { // SIMULADOR
            drawButton(ren, font, &btnBack, hoverBack);

            // Etiqueta del AFD actual
            char title[128];
            snprintf(title, sizeof(title), "Simulador %s (alfabeto: ", currentFile);
            for (int i = 0; i < afd.numSymbols; i++) {
                char t[8]; snprintf(t, sizeof(t), "%c%s", afd.symbols[i], (i==afd.numSymbols-1) ? "" : ",");
                strncat(title, t, sizeof(title)-strlen(title)-1);
            }
            strncat(title, ")", sizeof(title)-strlen(title)-1);
            drawText(ren, font, title, 50, 30, white);

            // Input + botón probar
            drawRoundedRect(ren, inputBox, inputBg, 10);
            drawButton(ren, font, &btnTest, hoverTest);

            SDL_Rect clip = {inputBox.x + 10, inputBox.y + 8, inputBox.w - 20, 24};
            SDL_RenderSetClipRect(ren, &clip);
            drawText(ren, font, input, inputBox.x + 10 - scrollX, inputBox.y + 8, white);
            SDL_RenderSetClipRect(ren, NULL);

            // Cursor de texto
            SDL_SetRenderDrawColor(ren, 200,200,200,255);
            int cursorX = 0;
            if (cursorPos > 0) {
                char tmp[MAX_STR]; strncpy(tmp, input, cursorPos); tmp[cursorPos] = '\0';
                TTF_SizeText(font, tmp, &cursorX, NULL);
            }
            SDL_RenderDrawLine(ren, inputBox.x + 10 + cursorX - scrollX, inputBox.y + 8,
                                    inputBox.x + 10 + cursorX - scrollX, inputBox.y + 32);

            // Trazas con scroll
            SDL_Rect viewArea = {50, 150, 420, 320};
            SDL_RenderSetClipRect(ren, &viewArea);
            int y = 150 - scrollY;
            for (int i = 0; i < steps; i++)
                drawText(ren, font, trace[i], 50, y + i * 30, white);
            SDL_RenderSetClipRect(ren, NULL);

            // Resultado
            if (accepted != -1) {
                SDL_Color c = accepted ? (SDL_Color){0,255,0} : (SDL_Color){255,0,0};
                drawText(ren, font, accepted ? "Cadena ACEPTADA" : "Cadena RECHAZADA", 50, 490, c);
                // Mostrar estados de aceptación
                char accbuf[256] = "Estados de aceptacion: ";
                for (int i = 0; i < afd.numAccept; i++) {
                    strncat(accbuf, afd.accept[i], sizeof(accbuf)-strlen(accbuf)-1);
                    if (i < afd.numAccept-1) strncat(accbuf, ", ", sizeof(accbuf)-strlen(accbuf)-1);
                }
                drawText(ren, font, accbuf, 50, 520, white);
            }

            // Tabla de transiciones con highlight del estado actual
            drawTransitionTable(ren, font, &afd, highlight);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}