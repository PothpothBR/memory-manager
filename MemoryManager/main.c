#define _CRT_SECURE_NO_WARNINGS
#pragma warning (disable: 5045)
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct Configuracoes {
    const char *replacement_policy, *writing_coherence, *allocation_policy;
    int associativity;
    char _[4];
};

struct Entradas {
    int cache_size, bloc_size, tc, trm, twm;
    int r_cpu_len;
};

struct Requisicoes {
    int ciclo, endereco;
    char tipo;
    char _[3];
};

struct Data {
    struct Entradas* entradas;
    struct Requisicoes* requisicoes;
    struct Configuracoes* conf;
};

struct Data* load_data(const char* file_name) {

    struct Configuracoes* conf = (struct Configuracoes*)calloc(1, sizeof(struct Configuracoes));
    if (!conf) {
        printf("\nerro ao alocar memoria");
        return 0;
    }

    conf->associativity = 8;
    conf->replacement_policy = "FIFO";
    conf->writing_coherence = "WT";
    conf->allocation_policy = "WNA";

    FILE *arq;
    struct Data *data = (struct Data*)calloc(1, sizeof(struct Data));
    struct Entradas *entradas = (struct Entradas*)calloc(1, sizeof(struct Entradas));

    if (!data || !entradas) {
        printf("\nerro ao alocar memoria");
        return 0;
    }

    arq = fopen(file_name, "r");
    if (arq == 0) {
        printf("\nnao foi possivel abrir o arquivo");
        return 0;
    }

    int res = fscanf(arq, "%i %i %i %i %i\n", &entradas->cache_size, &entradas->bloc_size, &entradas->tc, &entradas->trm, &entradas->twm);
    if (res < 0) {
        printf("falha ao ler o arquivo");
        return 0;
    }

    res = fscanf(arq, "%i", &entradas->r_cpu_len);
    if (res < 0) {
        printf("falha ao ler o arquivo");
        return 0;
    }
    struct Requisicoes *requisicoes = (struct Requisicoes*)calloc(entradas->r_cpu_len, sizeof(struct Requisicoes));

    if (!requisicoes) {
        printf("\nerro ao alocar memoria");
        return 0;
    }

    for (int i = 0; i < entradas->r_cpu_len; i++) {
        res = fscanf(arq,"%i %c %i",&requisicoes[i].ciclo,&requisicoes[i].tipo,&requisicoes[i].endereco);
        if (!res) {
            printf("falha ao ler o arquivo");
            return 0;
        }
    }

    data->entradas = entradas;
    data->requisicoes = requisicoes;
    data->conf = conf;
    return data;
}

void process(struct Data *data) {
    unsigned int clock = 0;

    for (int i = 0; i < data->entradas->r_cpu_len; i++) {
        clock = data->requisicoes->ciclo;
    }
}

int read() {

}

int write() {

}

void main(void) {
    struct Data *data;
    data = load_data("data.txt");
    char* cache = (char*)calloc(1, data->entradas->bloc_size);


    printf(
        "%i %i %i %s %s %s %i %i %i",
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

    free(data);
}