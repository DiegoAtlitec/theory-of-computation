// gcc afd_gui_visual.c -o afd_gui_visual -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE -L/opt/homebrew/lib -lSDL2 -lSDL2_ttf

//AFD Lenguaje de todas las cadenas sobre Σ = {a, b, c} donde cada cadena inicia con la subcadena ac y termina en la subcadena ab:


#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATES 10
#define MAX_SYMBOLS 3
#define MAX_STR 256

typedef struct {
    char symbols[MAX_SYMBOLS];
    char states[MAX_STATES][4];
    char trans[MAX_STATES][MAX_SYMBOLS][4];
    char start[4];
    char accept[4];
    int numStates;
} AFD;

typedef struct {
    SDL_Rect rect;
    SDL_Color color;
    char label[64];
} Button;

// ──────────────────────────────────────────────
// ░░░ UTILIDADES ░░░
// ──────────────────────────────────────────────
void drawCircle(SDL_Renderer *ren, int cx, int cy, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius))
                SDL_RenderDrawPoint(ren, cx + dx, cy + dy);
        }
    }
}

void drawRoundedRect(SDL_Renderer *ren, SDL_Rect rect, SDL_Color color, int radius) {
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, 255);
    SDL_Rect center = {rect.x + radius, rect.y, rect.w - 2 * radius, rect.h};
    SDL_RenderFillRect(ren, &center);
    SDL_Rect middle = {rect.x, rect.y + radius, rect.w, rect.h - 2 * radius};
    SDL_RenderFillRect(ren, &middle);
    drawCircle(ren, rect.x + radius, rect.y + radius, radius);
    drawCircle(ren, rect.x + rect.w - radius - 1, rect.y + radius, radius);
    drawCircle(ren, rect.x + radius, rect.y + rect.h - radius - 1, radius);
    drawCircle(ren, rect.x + rect.w - radius - 1, rect.y + rect.h - radius - 1, radius);
}

void drawText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!text || strlen(text) == 0) return;
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
// ░░░ LÓGICA DEL AFD ░░░
// ──────────────────────────────────────────────
int symbol_index(AFD *afd, char sym) {
    for (int i = 0; i < MAX_SYMBOLS; i++)
        if (afd->symbols[i] == sym) return i;
    return -1;
}

void loadAFD(AFD *afd, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { printf("❌ No se puede abrir %s\n", filename); exit(1); }

    for (int i = 0; i < MAX_SYMBOLS; i++)
        fscanf(f, " %c", &afd->symbols[i]);

    int idx = 0;
    while (idx < MAX_STATES) {
        if (fscanf(f, "%s", afd->states[idx]) != 1) break;
        for (int j = 0; j < MAX_SYMBOLS; j++)
            fscanf(f, "%s", afd->trans[idx][j]);
        idx++;
    }
    afd->numStates = idx;
    strcpy(afd->start, "q0");
    strcpy(afd->accept, "q4");
    fclose(f);
}

int simulateAFD(AFD *afd, const char *input, char trace[MAX_STR][32], int *steps, int path[MAX_STR]) {
    char current[4];
    strcpy(current, afd->start);
    *steps = 0;

    for (int i = 0; input[i]; i++) {
        int sidx = -1;
        for (int j = 0; j < afd->numStates; j++)
            if (strcmp(afd->states[j], current) == 0) { sidx = j; break; }

        int symi = symbol_index(afd, input[i]);
        if (sidx == -1 || symi == -1) return 0;

        sprintf(trace[*steps], "%s --%c--> %s", current, input[i], afd->trans[sidx][symi]);
        path[*steps] = sidx;
        (*steps)++;
        strcpy(current, afd->trans[sidx][symi]);
    }

    for (int j = 0; j < afd->numStates; j++)
        if (strcmp(afd->states[j], current) == 0)
            path[*steps] = j;

    return strcmp(current, afd->accept) == 0;
}

// ──────────────────────────────────────────────
// ░░░ INTERFAZ ░░░
// ──────────────────────────────────────────────
void drawButton(SDL_Renderer *ren, TTF_Font *font, Button *btn, int hovered) {
    SDL_Color bg = hovered ? (SDL_Color){90, 150, 220} : btn->color;
    drawRoundedRect(ren, btn->rect, bg, 10);
    SDL_Color white = {255, 255, 255};
    drawText(ren, font, btn->label, btn->rect.x + 20, btn->rect.y + 7, white);
}

void drawTransitionTable(SDL_Renderer *ren, TTF_Font *font, AFD *afd, int highlightIdx) {
    int startY = 220;
    int startX = 500;
    SDL_Color white = {255,255,255};
    SDL_Color yellow = {255,255,0};
    char buf[64];

    drawText(ren, font, "Tabla de transiciones:", startX, startY - 30, white);
    sprintf(buf, "    a      b      c");
    drawText(ren, font, buf, startX + 60, startY, white);
    startY += 30;

    for (int i = 0; i < afd->numStates; i++) {
        SDL_Color color = (i == highlightIdx) ? yellow : white;
        sprintf(buf, "%3s  %5s  %5s  %5s", afd->states[i],
                afd->trans[i][0], afd->trans[i][1], afd->trans[i][2]);
        drawText(ren, font, buf, startX, startY + i * 25, color);
    }
}

// ──────────────────────────────────────────────
// ░░░ MAIN ░░░
// ──────────────────────────────────────────────
int main() {
    AFD afd;
    loadAFD(&afd, "afd1.txt");

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *win = SDL_CreateWindow("Simulador AFD completo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 650, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/System/Library/Fonts/Supplemental/Arial.ttf", 22);
    if (!font) { printf("Error fuente: %s\n", TTF_GetError()); return 1; }

    Button btn = {{360, 40, 180, 40}, {70, 130, 200}, "Probar cadena"};
    SDL_Rect inputBox = {50, 40, 280, 40};

    SDL_Cursor *arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    SDL_Cursor *ibeam = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    SDL_Cursor *hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

    char input[MAX_STR] = "";
    int cursorPos = 0, running = 1, accepted = -1;
    char trace[MAX_STR][32];
    int steps = 0, highlight = -1, path[MAX_STR], focused = 0;
    int scrollY = 0, scrollX = 0;

    Uint32 lastTime = 0;
    int animStep = 0;

    SDL_StartTextInput();

    while (running) {
        SDL_Event e;
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        int hoverInput = pointInRect(mx, my, &inputBox);
        int hoverBtn = pointInRect(mx, my, &btn.rect);

        if (hoverBtn) SDL_SetCursor(hand);
        else if (hoverInput || focused) SDL_SetCursor(ibeam);
        else SDL_SetCursor(arrow);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                focused = pointInRect(e.button.x, e.button.y, &inputBox);
                if (pointInRect(e.button.x, e.button.y, &btn.rect)) {
                    accepted = simulateAFD(&afd, input, trace, &steps, path);
                    highlight = path[0];
                    scrollY = 0;
                    animStep = 0;
                    lastTime = SDL_GetTicks();
                }
            }
            else if (e.type == SDL_MOUSEWHEEL) {
                scrollY -= e.wheel.y * 30;
                if (scrollY < 0) scrollY = 0;
                int maxScroll = steps * 30 - 300;
                if (maxScroll < 0) maxScroll = 0;
                if (scrollY > maxScroll) scrollY = maxScroll;
            }
            else if (e.type == SDL_TEXTINPUT && focused) {
                char c = e.text.text[0];
                if (c == 'a' || c == 'b' || c == 'c') {
                    if (strlen(input) < MAX_STR - 1) {
                        memmove(&input[cursorPos + 1], &input[cursorPos],
                                strlen(input) - cursorPos + 1);
                        input[cursorPos] = c;
                        cursorPos++;
                    }
                }
            }
            else if (e.type == SDL_KEYDOWN && focused) {
                if (e.key.keysym.sym == SDLK_LEFT && cursorPos > 0)
                    cursorPos--;
                else if (e.key.keysym.sym == SDLK_RIGHT && cursorPos < strlen(input))
                    cursorPos++;
                else if (e.key.keysym.sym == SDLK_BACKSPACE && cursorPos > 0) {
                    memmove(&input[cursorPos - 1], &input[cursorPos],
                            strlen(input) - cursorPos + 1);
                    cursorPos--;
                } else if (e.key.keysym.sym == SDLK_DELETE && cursorPos < strlen(input)) {
                    memmove(&input[cursorPos], &input[cursorPos + 1],
                            strlen(input) - cursorPos);
                } else if (e.key.keysym.sym == SDLK_RETURN) {
                    accepted = simulateAFD(&afd, input, trace, &steps, path);
                    highlight = path[0];
                    scrollY = 0;
                    animStep = 0;
                    lastTime = SDL_GetTicks();
                }
            }
        }

        // 🔄 Animación de transición cada 500ms
        if (accepted != -1 && SDL_GetTicks() - lastTime > 500 && animStep <= steps) {
            highlight = path[animStep];
            animStep++;
            lastTime = SDL_GetTicks();
        }

        // Scroll horizontal input
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

        // Render
        SDL_SetRenderDrawColor(ren, 25, 25, 25, 255);
        SDL_RenderClear(ren);
        SDL_Color white = {255,255,255};
        SDL_Color inputBg = {50,50,50};
        drawRoundedRect(ren, inputBox, inputBg, 10);
        drawText(ren, font, "Introduce una cadena:", 50, 10, white);

        SDL_Rect clip = {inputBox.x + 10, inputBox.y + 8, inputBox.w - 20, 24};
        SDL_RenderSetClipRect(ren, &clip);
        drawText(ren, font, input, inputBox.x + 10 - scrollX, inputBox.y + 8, white);
        SDL_RenderSetClipRect(ren, NULL);

        SDL_SetRenderDrawColor(ren, 200,200,200,255);
        SDL_RenderDrawLine(ren, inputBox.x + 10 + cursorX - scrollX, inputBox.y + 8,
                           inputBox.x + 10 + cursorX - scrollX, inputBox.y + 32);

        drawButton(ren, font, &btn, hoverBtn);

        SDL_Rect viewArea = {50, 120, 400, 300};
        SDL_RenderSetClipRect(ren, &viewArea);
        int y = 120 - scrollY;
        for (int i = 0; i < steps; i++)
            drawText(ren, font, trace[i], 50, y + i * 30, white);
        SDL_RenderSetClipRect(ren, NULL);

        if (accepted != -1) {
            SDL_Color c = accepted ? (SDL_Color){0,255,0} : (SDL_Color){255,0,0};
            drawText(ren, font, accepted ? "Cadena ACEPTADA" : "Cadena RECHAZADA",
                     50, 450, c);
        }

        drawTransitionTable(ren, font, &afd, highlight);
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