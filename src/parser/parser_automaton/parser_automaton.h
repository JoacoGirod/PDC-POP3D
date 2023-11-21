#ifndef __PARSER_AUTOMATON_H__
#define __PARSER_AUTOMATON_H__

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>
#include <errno.h>

#define N(x) (sizeof(x) / sizeof((x)[0]))

#define IS_LETTER(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define IS_PRINTABLE(c) (c >= 0x20 && c <= 0x7E)
#define IS_SPACE(c) (c == ' ' || c == '\t')
#define IS_CR(c) (c == '\r')
#define IS_LF(c) (c == '\n')
#define IS_ANY(c) (true)
#define IS_ALPHANUMERIC(c) (isalnum(c))
typedef enum parser_state
{
    PARSER_READING = 0,
    PARSER_ACTION,
    PARSER_FINISHED,
    PARSER_ERROR
} parser_state;

typedef bool (*parser_condition)(uint8_t c);
typedef parser_state (*parser_action)(void *data, uint8_t c);
typedef void *(*parser_automaton_init)(void);
typedef void *(*parser_automaton_copy)(void *data);
typedef void (*parser_automaton_reset)(void *data);
typedef void (*parser_automaton_destroy)(void *data);

inline static bool is_letter(uint8_t c)
{
    return IS_LETTER(c);
}
inline static bool is_printable(uint8_t c)
{
    return IS_PRINTABLE(c);
}
inline static bool is_space(uint8_t c)
{
    return IS_SPACE(c);
}
inline static bool is_cr(uint8_t c)
{
    return IS_CR(c);
}
inline static bool is_lf(uint8_t c)
{
    return IS_LF(c);
}
inline static bool is_any(uint8_t c)
{
    return IS_ANY(c);
}
inline static bool is_alphanumeric(uint8_t c)
{
    return IS_ALPHANUMERIC(c);
}

/** describe una transición entre estados  */
struct parser_state_transition
{
    /** condición: un parser_condition */
    parser_condition when;
    /** descriptor del estado destino cuando se cumple la condición */
    unsigned dest;
    /** acción a ejecutar cuando se cumple la condición */
    parser_action action;
};

/** declaración completa de una máquina de estados */
typedef struct parser_automaton
{
    /** cantidad de estados */
    const unsigned states_count;
    /** por cada estado, sus transiciones */
    const struct parser_state_transition **states;
    /** cantidad de estados por transición */
    const size_t *states_n;

    /** estado inicial */
    const unsigned start_state;

    /** función para inicializar la estructura data */
    /** devuelve NULL si no se pudo inicializar */
    /** si no se necesita data, se puede pasar NULL */
    parser_automaton_init init;

    /** función para devolver una copia de la estructura data */
    /** devuelve NULL si no se pudo realizar la copia */
    /** si no se necesita data, se puede pasar NULL */
    parser_automaton_copy copy;

    /** función para resetear la estructura data */
    /** si no se necesita data, se puede pasar NULL */
    parser_automaton_reset reset;

    /** función para destruir la estructura data */
    /** si no se necesita data, se puede pasar NULL */
    parser_automaton_destroy destroy;
} parser_automaton;

#endif
