/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#include "nsStringGlue.h"
#include "jsmn.h"

#define JSON_TOKENS 10

const char *KEYS[] = {"idKey","accountKey","type"};

static
jsmntok_t *
json_tokenise(const char *js, size_t len)
{
  jsmn_parser parser;
  jsmn_init(&parser);

  unsigned int n = JSON_TOKENS;
  jsmntok_t *tokens = (jsmntok_t*)malloc(sizeof(jsmntok_t) * n);

  if (!tokens)
    return NULL;

  int ret = jsmn_parse(&parser, js, len, tokens, n);

  while (ret == JSMN_ERROR_NOMEM)
  {
    n = n * 2 + 1;
    tokens = (jsmntok_t *)realloc(tokens, sizeof(jsmntok_t) * n);

    if (!tokens)
      return NULL;
    
    ret = jsmn_parse(&parser, js, len, tokens, n);
  }

  if (ret == JSMN_ERROR_INVAL)
    goto Error;
  if (ret == JSMN_ERROR_PART)
    goto Error;
  return tokens;
Error:
  if (tokens) free(tokens);
  return NULL;
}

static
bool
json_token_streq(const char *js, jsmntok_t *t, const char *s)
{
  return (strncmp(js + t->start, s, t->end - t->start) == 0
          && strlen(s) == (size_t) (t->end - t->start));
}

nsresult
parse_params(const nsCString & jsonData,
             nsCString & idKey,
             nsCString & accountKey,
             nsCString & type)
{
  const char *js = jsonData.get();

  jsmntok_t *tokens = json_tokenise(js, jsonData.Length());

  if (!tokens)
    return NS_ERROR_FAILURE;
  
  typedef enum { START, KEY, PRINT, SKIP, STOP } parse_state;
  parse_state state = START;

  nsresult rv = NS_OK;
  size_t key_index = -1;

  for (size_t i = 0, j = 1; j > 0; i++, j--)
  {
    jsmntok_t *t = &tokens[i];

    j += t->size;

    switch (state)
    {
    case START:
      if (t->type != JSMN_OBJECT)
        goto DIE;

      state = KEY;

      break;

    case KEY:
      if (t->type != JSMN_STRING)
        goto DIE;

      state = SKIP;

      for (size_t ii = 0; ii < sizeof(KEYS)/sizeof(const char *); ii++)
      {
        if (json_token_streq(js, t, KEYS[ii]))
        {
          state = PRINT;
          key_index = ii;
          break;
        }
      }

      break;

    case SKIP:
      if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE)
        goto DIE;

      state = KEY;

      break;

    case PRINT: {
      if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE)
        goto DIE;

      switch(key_index) {
      case 0:
        idKey.Assign(&js[t->start], t->end - t->start);
        break;
      case 1:
        accountKey.Assign(&js[t->start], t->end - t->start);
        break;
      case 2:
        type.Assign(&js[t->start], t->end - t->start);
        break;
      default:
        break;
      }

      state = KEY;
    }
      break;

    case STOP:
      // Just consume the tokens
      break;

    default:
      goto DIE;
    }
  }

RET:
  if (tokens) free(tokens); 
  return rv;
DIE:
  rv = NS_ERROR_FAILURE;
  goto RET;
}
