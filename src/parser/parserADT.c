#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include "parserADT.h"
// #include "pop3_functions.h"

#define CAPA_MESSAGE "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"
#define CAPA_AUTHORIZED_MESSAGE "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"
// #define BASE_DIR "/tmp/Maildir"
#define MAX_FILE_PATH 700
#define MAX_COMMAND_LENGTH 255
#define BUFFER_SIZE 1024

parserADT parser_init(const parser_automaton *def)
{
    parserADT p = calloc(1, sizeof(parserCDT));
    if (p == NULL || errno == ENOMEM)
    {
        printf("Error creating parserADT");
        return NULL;
    }
    if (def->init != NULL)
    {
        p->data = def->init();
        if (p->data == NULL)
        {
            printf("Error initializing parser data structure");
            free(p);
            return NULL;
        }
    }
    p->def = def;
    p->state = def->start_state;
    p->parser_state = PARSER_READING;
    return p;
}

void parser_destroy(parserADT p)
{
    if (p != NULL)
    {
        if (p->def->destroy != NULL)
        {
            p->def->destroy(p->data);
        }
        free(p);
    }
}

void parser_reset(parserADT p)
{
    p->state = p->def->start_state;
    p->parser_state = PARSER_READING;
    if (p->def->reset != NULL)
    {
        p->def->reset(p->data);
    }
}

void *parser_get_data(parserADT p)
{
    if (p->def->copy == NULL)
    {
        return NULL;
    }
    return p->def->copy(p->data);
}

parser_state parser_feed(parserADT p, uint8_t c)
{
    // Se obtienen las transiciones del estado actual
    const struct parser_state_transition *state = p->def->states[p->state];
    // Se obtiene la cantidad de transiciones del estado actual
    const size_t n = p->def->states_n[p->state];
    // Flag para indicar si se encontró una transición que con la condición
    bool matched = false;
    for (size_t i = 0; i < n && !matched; i++)
    {
        matched = state[i].when(c);
        if (matched)
        {
            p->state = state[i].dest;
            parser_state resp = state[i].action(p->data, c);
            if (resp == PARSER_FINISHED)
            {
                return (p->parser_state == PARSER_ERROR) ? p->parser_state : resp;
            }
            else if (resp == PARSER_ERROR)
            {
                p->parser_state = PARSER_ERROR;
            }
            else if (resp == PARSER_ACTION)
            {
                return resp;
            }
        }
    }
    return PARSER_READING;
}

bool has_argument(const char *arg)
{
    return strlen(arg) != 0;
}

bool can_have_argument(const char *arg)
{
    return true;
}

bool hasnt_argument(const char *arg)
{
    return arg == NULL || strlen(arg) == 0;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;
    while (n-- != 0 && tolower((unsigned char)*s1) == tolower((unsigned char)*s2))
    {
        if (n == 0 || *s1 == '\0' || *s2 == '\0')
            break;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

size_t u_strlen(const uint8_t *s)
{
    size_t i = 0;
    while (s[i] != '\0')
    {
        i++;
    }
    return i;
}
