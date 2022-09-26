
/**   Para casos de compilação com o visual studio, o warning 5045 esta desabilitado
*   pois o mesmo é referente a um recurso externo que não se aplica ao código abaixo
*   a flag _CRT_SECURE_NO_WARNINGS permite usar as funções padrão da biblioteca C (stdlib.h).
* 
*     O programa consiste em uma versão minimalista de um simulador de cache, sendo desenvolvido
*   de forma estruturada e monolítico.
* 
* @Author Bruno Dalagnol, Renato Leal
* @Created 22/09/2021
*/

#define _CRT_SECURE_NO_WARNINGS
#define verify(res, err) if (!(res)) { printf(err); exit(-1); }
#pragma warning (disable: 5045)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>


typedef unsigned int uint;

/* genéricos */

// alloca e verifica o estado da memoria, em caso de falha encerra o programa
void* safeAlloc(uint count, uint size) {
    void* res = calloc(count, size);
    verify(res, "\nnao foi possivel alocar memoria");
    return res;
}

// realloca e verifica o estado da memoria, em caso de falha encerra o programa
void* safeRealloc(void* buffer, uint count, uint size) {
    buffer = realloc(buffer, count*size);
    verify(buffer, "\nnao foi possivel alocar memoria");
    return buffer;
}

/* desenho grafico */

// representa o ascii map para desenho no terminal
typedef struct _Screen {
    char* buffer;
    unsigned int len;
    unsigned int offset;
}Screen;

Screen screen;

void initScreen(uint size) {
    screen.buffer = (char*)safeAlloc(size, sizeof(char));
    screen.len = size;
    screen.offset = 0;
}

void closeScreen(void) {
    free(screen.buffer);
}

void draw(const char* format, ...) {
    char buffer[1024] = {0};

    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);

    uint
        buffer_len = (uint) strlen(buffer),
        future_offset = screen.offset + buffer_len;

    if (screen.len < future_offset) {
        screen.len = future_offset+256;
        screen.buffer = (char*)safeRealloc(screen.buffer, screen.len, sizeof(char));
    }

    strcpy(screen.buffer + screen.offset, buffer);
    screen.offset = future_offset;

    va_end(args);
}

void drawl(const char* format, ...) {
    char buffer[1024] = { 0 };

    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);

    unsigned int
        buffer_len = (uint)strlen(buffer) + 1,
        future_offset = screen.offset + buffer_len;

    buffer[buffer_len - 1] = '\n';

    if (screen.len < future_offset) {
        screen.len = future_offset + 256;
        screen.buffer = (char*)safeRealloc(screen.buffer, screen.len, sizeof(char));
    }

    strcpy(&screen.buffer[screen.offset], buffer);
    screen.offset = future_offset;

    va_end(args);
}

// realiza a rotina de limpesa e desenho do buffer no terminal
void flipScreen(void) {
#ifdef _WIN64
    system("cls");
#else
    system("clear");
#endif
    printf("%s", screen.buffer);
    fflush(stdout);
    memset(screen.buffer, ' ', screen.len);
    screen.offset = 0;
}


/* simular a cache */

// Configurações para a execução da cache simulada.
typedef struct _Configuracoes {
    const char *replacement_policy, *writing_coherence, *allocation_policy;
    uint associativity;
    char _[4];
}Configuracoes;

// definições que estruturam a cache
typedef struct _Entradas {
    uint
        cache_size,
        bloc_size,
        tc,
        trm,
        twm,
        r_cpu_len;
    uint offset_unsift_mask;
    uint index_unsift_mask;
    uint offset_mask;
    uint index_mask;
}Entradas;

// entradas a serem executadas
typedef struct _Requisicoes {
    uint ciclo, endereco;
    char tipo;
    char _[3];
}Requisicoes;

// todos os dados fornecidos a aplicação
typedef struct _Data {
    Entradas* entradas;
    Configuracoes* conf;
    Requisicoes* requisicoes;
}Data;

// resultados obtidos durante a execução
typedef struct _Results {
    uint
        time_counter,
        hits,
        misses;

}Results;

// endereco formatado
typedef struct _Address {
    uint
        offset,
        index,
        tag;

}Address;

// representa uma linha de cache
typedef struct _CacheLine {
    uint
        * data,
        valid,
        tag,
        addr,
        age;

}CacheLine;

// representa um conjunto associativo da cache
typedef struct _CacheSet {
    CacheLine* lines;
    uint index;
    char _[4];
}CacheSet;

// representa a cache em sua totalidade
typedef struct _Cache {
    CacheSet* sets;
    uint sets_len,
        lines_len,
        line_size,
        size;
}Cache;

// cria e inicializa o cache
Cache* digestCache(Data* data) {
    Cache* cache = (Cache*)safeAlloc(1, sizeof(Cache));

    cache->size = data->entradas->cache_size;
    cache->line_size = data->entradas->bloc_size;
    cache->lines_len = data->conf->associativity;
    cache->sets_len = cache->size / cache->line_size / cache->lines_len;

    cache->sets = (CacheSet*)safeAlloc(cache->sets_len, sizeof(CacheSet));

    for (uint i = 0; i < cache->sets_len; i++) {
        cache->sets[i].lines = (CacheLine*)safeAlloc(cache->lines_len, sizeof(CacheLine));
        cache->sets[i].index = i;
        for (uint e = 0; e < cache->lines_len; e++) {
            cache->sets[i].lines[e].data = (uint*)safeAlloc(cache->line_size, sizeof(uint));
            cache->sets[i].lines[e].tag = 0;
            cache->sets[i].lines[e].valid = 0;
        }
    }
    
    data->entradas->offset_mask = (cache->line_size % 2) ? cache->line_size : cache->line_size - 1;
    data->entradas->offset_unsift_mask = (uint) log2(data->entradas->offset_mask+1);

    data->entradas->index_mask = (cache->sets_len % 2) ? cache->sets_len : cache->sets_len - 1;
    data->entradas->index_unsift_mask = (uint) log2(data->entradas->index_mask+1);
    
    return cache;
}

// pega o endereço e quebra-o em seus dados
void digestAddress(Address* address, Data* data, Requisicoes* req) {
    uint addr       = req->endereco;
    address->offset = addr &  data->entradas->offset_mask;
    addr            = addr >> data->entradas->offset_unsift_mask;
    address->index  = addr &  data->entradas->index_mask;
    addr            = addr >> data->entradas->index_unsift_mask;
    address->tag    = addr;
}

// carrega os dados que alimentam a aplicação
Data* load_data(const char* file_name) {

    Configuracoes* conf = (Configuracoes*)safeAlloc(1, sizeof(Configuracoes));

    conf->associativity = 8;
    conf->replacement_policy = "FIFO";
    conf->writing_coherence = "WT";
    conf->allocation_policy = "WNA";

    FILE *arq;
    Data *data = (Data*)safeAlloc(1, sizeof(Data));
    Entradas *entradas = (Entradas*)safeAlloc(1, sizeof(Entradas));

    arq = fopen(file_name, "r");
    if (fscanf(
            arq, "%u %u %u %u %u\n",
            &entradas->cache_size,
            &entradas->bloc_size,
            &entradas->tc,
            &entradas->trm,
            &entradas->twm
    ) < 0) {
        printf("falha ao ler o arquivo");
        exit(-1);
    }
    if (fscanf(arq, "%u", &entradas->r_cpu_len) < 0) {
        printf("falha ao ler o arquivo");
        exit(-1);
    }

    Requisicoes *requisicoes = (Requisicoes*)safeAlloc(entradas->r_cpu_len, sizeof(Requisicoes));

    for (uint i = 0; i < entradas->r_cpu_len; i++) {
        if (!fscanf(arq, "%u %c %u", &requisicoes[i].ciclo, &requisicoes[i].tipo, &requisicoes[i].endereco)) {
            printf("falha ao ler o arquivo");
            exit(-1);
        }
    }

    fclose(arq);
    data->entradas = entradas;
    data->requisicoes = requisicoes;
    data->conf = conf;
    return data;
}

// libera os dados da aplicação
void freeData(Data* data) {
    free(data->requisicoes);
    free(data->conf);
    free(data->entradas);
}

void freeCache(Cache* cache) {
    for (uint i = 0; i < cache->sets_len; i++) {
        for (uint e = 0; e < cache->lines_len; e++) {
            free(cache->sets[i].lines[e].data);
        }
        free(cache->sets[i].lines);
    }
    free(cache->sets);
}

// realiza a execução da aplicação
void process(Data* data, Cache* cache) {
    Address* address = (Address*)safeAlloc(1, sizeof(Address));;
    Results* results = (Results*)safeAlloc(1, sizeof(Results));

    int is_hit = 0;
    for (uint i = 0; i < data->entradas->r_cpu_len; i++) {
        digestAddress(address, data, &data->requisicoes[i]);

        // aumenta o ciclo de idade para todas as linhas
        for (uint e = 0; e < cache->sets_len; e++) {
            for (uint j = 0; j < cache->lines_len; j++) {
                cache->sets[e].lines[j].age++;
            }
        }

        // inicia a busca dos dados para cada conjunto associativo
        for (uint e = 0; e < cache->sets_len; e++) {
            if (cache->sets[e].index == address->index) {
                is_hit = 0;

                // inicia a busca dos dados pelas linhas do conjunto
                for (uint j = 0; j < cache->lines_len; j++) {
                    // se encontar verifica se possui tag
                    if (cache->sets[e].lines[j].valid && cache->sets[e].lines[j].tag == address->tag) {
                        is_hit = 1;
                        results->hits++;

                        // calcula os custos de tempo da operação
                        if (data->requisicoes[i].tipo == 'R') { results->time_counter++; }
                        else { results->time_counter += data->entradas->twm; }

                        break;
                    }
                }

                // se nao encontrar
                if (!is_hit) {
                    results->misses++;
                    // se for leitura (WNA policity)
                    if (data->requisicoes[i].tipo == 'R') {
                        uint
                            has_invalid = 0,
                            w_age = 0,
                            w_j = 0;

                        // verifica se possui linhas vazias para alocar
                        for (uint j = 0; j < cache->lines_len; j++) {
                            if (!cache->sets[e].lines[j].valid) {
                                w_j = j;
                                has_invalid = 1;
                                break;
                            }
                        }

                        // se nao possuir substitui a mais velha
                        if (!has_invalid) {
                            for (uint j = 0; j < cache->lines_len; j++) {
                                if (cache->sets[e].lines[j].age > w_age) {
                                    w_j = j;
                                    w_age = cache->sets[e].lines[j].age;
                                }
                            }
                        }

                        // seta os dados na linha escolhida de cache
                        cache->sets[e].lines[w_j].age = 0;
                        cache->sets[e].lines[w_j].valid = 1;
                        cache->sets[e].lines[w_j].tag = address->tag;
                        cache->sets[e].lines[w_j].addr = data->requisicoes[i].endereco;

                        // pucha todo o conjunto local de dados (tamanho maximo e minimo do offset)
                        uint addr_start = 0;
                        for (uint l = 0; l < cache->line_size; l++) {
                            cache->sets[e].lines[w_j].data[l] = addr_start++;
                        }

                        // calcula os custos de tempo da operação
                        results->time_counter += data->entradas->tc + data->entradas->trm;
                    }
                    else {
                        // calcula os custos de tempo da operação
                        results->time_counter += data->entradas->twm;
                    }

                }
                break;
            }
        }

        // desenha as rotinas graficas
        drawl(
            "%u %u %u %s %s %s %u %u %u",
            data->entradas->cache_size,
            data->conf->associativity,
            data->entradas->bloc_size,
            data->conf->replacement_policy,
            data->conf->writing_coherence,
            data->conf->allocation_policy,
            data->entradas->tc,
            data->entradas->trm,
            data->entradas->twm
        );
        drawl("%u", results->time_counter);
        drawl("%u %u", results->hits, results->misses);
        drawl("%u", cache->sets_len);

        for (uint e = 0; e < cache->sets_len; e++) {
            for (uint j = 0; j < cache->lines_len; j++) {
                if (!cache->sets[e].lines[j].valid) {
                    draw("%16s", "NULL");
                }
                else {
                    draw("%16u", cache->sets[e].lines[j].addr);
                }
            }
            drawl("");
        }

        flipScreen();
    }

    free(address);
    free(results);
}

// entry point da aplicação
void main(uint argc, char **argv) {

    verify(argc > 1, "nome do arquivo nao informado");
    verify(argc < 3, "o unico argumento posivel é o nome do arquivo");

    initScreen(256);
    Data *data = load_data(argv[1]);
    Cache* cache = digestCache(data);

    process(data, cache);

    closeScreen();
    freeData(data);
    freeCache(cache);
}