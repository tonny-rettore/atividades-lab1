#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "janela.h"
#include <string.h>

typedef struct
{
    ponto_t p;
    int altura;
    int largura;
} tamanho;

typedef struct
{
    int id;
    char texto[500];
    char etiqueta[4];
    cor_t cor;
    tamanho tamanho;
} Nota;

typedef struct
{
    Nota *v_notas;
    int proximo_id;
    int total_notas;
    int capacidade;
    char texto_busca[500];

    ponto_t cursor;

    ponto_t cursor_editando_inicio;
    ponto_t cursor_editando_fim;

    int caractere_atual_editando;

    int nota_corrente;
    bool existe_ultima_removida;
    Nota ultima_nota_removida;

    int cor_atual_editando;
    cor_t copia_cor;
    int digito_atual_cor;

    char copia_texto[500];
    char copia_etiqueta[4];
} Sistema;

void grava_nota_arq(Nota *v_notas, int total_notas)
{
    FILE *arq;
    arq = fopen("notas", "w");

    if (arq == NULL)
    {
        printf("Não foi possível abrir o arquivo 'notas'");
        return;
    }

    for (int i = 0; i < total_notas; i++)
    {
        fprintf(arq, "%s %d %d %d %d %d %d %d \"%s\"\n",
                v_notas[i].etiqueta,
                (int)v_notas[i].cor.vermelho,
                (int)v_notas[i].cor.verde,
                (int)v_notas[i].cor.azul,
                (int)v_notas[i].tamanho.p.x,
                (int)v_notas[i].tamanho.p.y,
                v_notas[i].tamanho.largura,
                v_notas[i].tamanho.altura,
                v_notas[i].texto);
    }

    fclose(arq);
}

// copia a linha para o arquivo de linhas com problemas
void registra_linha_problema(char *linha)
{
    FILE *arq;
    arq = fopen("notas_problemas", "a");
    if (arq == NULL)
        return;
    fprintf(arq, "%s\n", linha);
    fclose(arq);
}

void le_nota_arq(Sistema *s)
{
    FILE *arq;
    arq = fopen("notas", "r");

    if (arq == NULL)
        return;

    char linha[2000];

    while (fgets(linha, sizeof(linha), arq) != NULL)
    {
        char etiqueta[4];
        int r, g, b, x, y, largura, altura;
        char texto[500];
        int consumidos;

        // esse %n ele vai retornar onde parou, tipo quantos caracteres foram lidos
        int lidos = sscanf(linha, "%3[A-Z0-9] %d %d %d %d %d %d %d \"%499[^\"]\"%n", etiqueta, &r, &g, &b, &x, &y, &largura, &altura, texto, &consumidos);

        if (lidos != 9)
        {
            registra_linha_problema(linha);
            continue;
        }

        if (strlen(etiqueta) != 3)
        {
            registra_linha_problema(linha);
            continue;
        }

        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
        {
            registra_linha_problema(linha);
            continue;
        }

        bool texto_invalido = false;

        for (int i = 0; texto[i] != '\0'; i++)
        {
            unsigned char c = texto[i];

            if (c < 32 || c > 126 || c == '"')
            {
                texto_invalido = true;
                break;
            }
        }

        if (texto_invalido)
        {
            registra_linha_problema(linha);
            continue;
        }

        bool texto_truncado = (linha[consumidos - 1] != '"');
        if (texto_truncado)
        {
            registra_linha_problema(linha);
        }

        if (s->total_notas >= s->capacidade)
        {
            int nova_capacidade = s->capacidade + 10;
            Nota *temp = realloc(s->v_notas, nova_capacidade * sizeof(Nota));
            if (temp == NULL)
            {
                printf("Erro de memoria.\n");
                fclose(arq);
                return;
            }
            s->v_notas = temp;
            s->capacidade = nova_capacidade;
        }

        Nota n;
        n.id = s->proximo_id++;
        strcpy(n.etiqueta, etiqueta);
        n.cor.vermelho = r;
        n.cor.verde = g;
        n.cor.azul = b;
        n.cor.opacidade = 255;
        n.tamanho.p.x = x;
        n.tamanho.p.y = y;
        n.tamanho.largura = largura;
        n.tamanho.altura = altura;
        strcpy(n.texto, texto);

        s->v_notas[s->total_notas] = n;
        s->total_notas++;
    }

    fclose(arq);
}

// DESENHA NOTA
void desenha_nota(Nota *n, int corrente, int i)
{
    // parametros pro desenho do retangulo
    retangulo_t retangulo;
    retangulo.inicio.x = n->tamanho.p.x;
    retangulo.inicio.y = n->tamanho.p.y;
    retangulo.tamanho.largura = n->tamanho.largura;
    retangulo.tamanho.altura = n->tamanho.altura;

    cor_t cor_retangulo;
    cor_retangulo.vermelho = n->cor.vermelho / 255.0;
    cor_retangulo.verde = n->cor.verde / 255.0;
    cor_retangulo.azul = n->cor.azul / 255.0;
    cor_retangulo.opacidade = 1;

    cor_t contorno_normal;
    contorno_normal.vermelho = 1;
    contorno_normal.verde = 1;
    contorno_normal.azul = 1;
    contorno_normal.opacidade = 1;

    cor_t contorno_corrente;
    contorno_corrente.vermelho = 1;
    contorno_corrente.verde = 1;
    contorno_corrente.azul = 0;
    contorno_corrente.opacidade = 1;

    // parametros pro texto da nota
    ponto_t posicao_texto_inicial;
    posicao_texto_inicial.x = n->tamanho.p.x + 5;
    posicao_texto_inicial.y = n->tamanho.p.y + 35;

    // calculo media pra ver se o texto sera preto ou branco
    float media = (n->cor.vermelho + n->cor.verde + n->cor.azul) / 3.0;

    cor_t cor_texto;
    if (media > 120)
    {
        cor_texto.vermelho = 0;
        cor_texto.verde = 0;
        cor_texto.azul = 0;
        cor_texto.opacidade = 1;
    }
    else
    {
        cor_texto.vermelho = 1;
        cor_texto.verde = 1;
        cor_texto.azul = 1;
        cor_texto.opacidade = 1;
    }

    // parametros etiqueta nota
    ponto_t posicao_etiqueta;
    posicao_etiqueta.x = n->tamanho.p.x + n->tamanho.largura - 40;
    posicao_etiqueta.y = n->tamanho.p.y + 15;

    // cor do texto da etiqueta sera o mesmo do texto

    // chamadas funcoes que desenham
    if (corrente == i)
    {
        j_retangulo(retangulo, 4, contorno_corrente, cor_retangulo);
    }
    else
    {
        j_retangulo(retangulo, 1, contorno_normal, cor_retangulo);
    }

    j_seleciona_fonte(NULL, 16);
    j_texto(posicao_etiqueta, cor_texto, n->etiqueta);

    // logica pra desenhar o texto com quebra de linha
    j_seleciona_fonte(NULL, 12);
    ponto_t posicao_texto = posicao_texto_inicial;

    // fiz de forma dinamica pq se aumentar o tamanho pode ser que caiba mais em uma linha
    int qtd_caracteres_linha = 100;
    char *linha = malloc(qtd_caracteres_linha * sizeof(char));
    if (linha == NULL)
    {
        printf("Erro de memoria, experiemente fechar alguns programas.\n");
        return;
    }

    linha[0] = '\0';
    retangulo_t ret_linha = j_texto_contorno(linha);

    int altura_disponivel = n->tamanho.altura - 40; // altura disponivel pra texto
    int altura_texto = 0;

    int x = 0;
    for (int i = 0; (n->texto[i] != '\0') && (altura_texto < altura_disponivel); i++)
    {
        // realloc caso caiba mais caracteres na linha
        if (x + 1 >= qtd_caracteres_linha)
        {
            qtd_caracteres_linha += 30;
            char *temp = realloc(linha, qtd_caracteres_linha * sizeof(char));
            if (temp == NULL)
            {
                free(linha);
                printf("Erro de memoria.\n");
                return;
            }
            linha = temp;
        }

        linha[x] = n->texto[i];
        linha[x + 1] = '\0';

        ret_linha = j_texto_contorno(linha);

        // ve se quebra a linha
        if (ret_linha.tamanho.largura > n->tamanho.largura - 10)
        {
            linha[x] = '\0'; // remove o caractere que não coube

            j_texto(posicao_texto, cor_texto, linha);

            posicao_texto.y += 15;
            altura_texto += 15;

            linha[0] = '\0';
            x = 0;
            i--; // volta 1 pra nao perder esse caractere q vai ser escreito proxima linha
        }
        else
        {
            x++;
        }
    }
    if (x > 0)
    {
        j_texto(posicao_texto, cor_texto, linha);
    }
    free(linha);
}

void desenha_cursor(ponto_t *cursor)
{

    circulo_t circulo_cursor;
    circulo_cursor.centro.x = cursor->x;
    circulo_cursor.centro.y = cursor->y;
    circulo_cursor.raio = 4;

    cor_t cor_cursor;
    cor_cursor.vermelho = 1;
    cor_cursor.verde = 0;
    cor_cursor.azul = 0;
    cor_cursor.opacidade = 0.5;

    j_circulo(circulo_cursor, 1, cor_cursor, cor_cursor);
}

bool cursor_ta_na_nota(Nota *n, ponto_t *cursor)
{
    if ((cursor->x >= n->tamanho.p.x &&
         cursor->x <= n->tamanho.p.x + n->tamanho.largura &&
         cursor->y >= n->tamanho.p.y &&
         cursor->y <= n->tamanho.p.y + n->tamanho.altura) == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// MODOS

// MODO PRINCIPAL
// FUNCOES MODO PRINCIPAL

// move nota corrente pro início do vetor
void move_corrente_inicio(Sistema *s)
{
    if (s->nota_corrente >= 0)
    {
        Nota temp = s->v_notas[s->nota_corrente];
        for (int i = s->nota_corrente; i > 0; i--)
        {
            s->v_notas[i] = s->v_notas[i - 1];
        }

        s->v_notas[0] = temp;
        s->nota_corrente = 0;
    }
}

// move nota corrente pro fim do vetor
void move_corrente_fim(Sistema *s)
{
    if (s->nota_corrente >= 0)
    {
        Nota temp = s->v_notas[s->nota_corrente];
        for (int i = s->nota_corrente; i < s->total_notas - 1; i++)
        {
            s->v_notas[i] = s->v_notas[i + 1];
        }

        s->v_notas[s->total_notas - 1] = temp;
        s->nota_corrente = s->total_notas - 1;
    }
}

// remove a nota corrente, se houver; ela passa a ser a "última nota removida";
void remove_nota_corrente(Sistema *s)
{
    if (s->nota_corrente >= 0)
    {
        s->ultima_nota_removida = s->v_notas[s->nota_corrente];

        for (int i = s->nota_corrente; i < s->total_notas - 1; i++)
        {
            s->v_notas[i] = s->v_notas[i + 1];
        }

        s->total_notas = s->total_notas - 1;
        s->nota_corrente = -1;
        s->existe_ultima_removida = true;

        // REALOCACAO PARA BAIXO
        if (s->total_notas <= s->capacidade - 20 && s->capacidade > 10)
        {
            int nova_capacidade = s->capacidade - 10;
            if (nova_capacidade < 10)
                nova_capacidade = 10;

            Nota *temp = realloc(s->v_notas, nova_capacidade * sizeof(Nota));
            if (temp != NULL)
            {
                s->v_notas = temp;
                s->capacidade = nova_capacidade;
            }
            // se realloc falhar, mantém o bloco maior — sem problema
        }
    }
}

// reinsere a "última nota removida", se houver; ela deve ser colocada no final do vetor.
// Após a inserção, passa a não existir "última nota removida".
void reinsere_nota_removida(Sistema *s)
{
    if (s->existe_ultima_removida == true)
    {
        // REALOCACAO
        if (s->total_notas >= s->capacidade)
        {
            int nova_capacidade = s->capacidade + 10;

            Nota *temp = realloc(s->v_notas, nova_capacidade * sizeof(Nota));

            if (temp == NULL)
            {
                printf("Erro de memoria.\n");
                return;
            }

            s->v_notas = temp;
            s->capacidade = nova_capacidade;
        }

        // atualiza o ponto do retangulo da nota para a posicao do cursor
        s->ultima_nota_removida.tamanho.p.x = s->cursor.x;
        s->ultima_nota_removida.tamanho.p.y = s->cursor.y;

        // INSERCAO
        s->v_notas[s->total_notas] = s->ultima_nota_removida;
        s->existe_ultima_removida = false;
        s->total_notas++;
    }
}

// cria uma nova nota, no final do vetor, com conteúdo padrão;
void cria_nota_padrao(Sistema *s)
{
    Nota nota;

    // ID
    nota.id = s->proximo_id;
    s->proximo_id++;

    // TEXTO
    strcpy(nota.texto, "Texto texto texto texto texto texto texto texto texto exto texto texto texto texto texto texto texto texto exto texto texto texto texto texto texto texto texto exto texto texto texto texto texto texto texto textoexto texto texto texto texto texto texto texto textoexto texto texto texto texto texto texto texto textoexto texto texto texto texto texto texto texto textoexto texto texto texto texto texto texto texto textoexto texto texto texto texto texto texto texto texto");

    // ETIQUETA
    strcpy(nota.etiqueta, "AAA");

    // COR
    nota.cor.vermelho = 255;
    nota.cor.verde = 229;
    nota.cor.azul = 127;
    nota.cor.opacidade = 255;

    // TAMANHO
    nota.tamanho.largura = 230;
    nota.tamanho.altura = 170;

    nota.tamanho.p.x = s->cursor.x;
    nota.tamanho.p.y = s->cursor.y;

    // REALOCACAO
    if (s->total_notas >= s->capacidade)
    {
        int nova_capacidade = s->capacidade + 10;

        Nota *temp = realloc(s->v_notas, nova_capacidade * sizeof(Nota));

        if (temp == NULL)
        {
            printf("Erro de memoria.\n");
            return;
        }

        s->v_notas = temp;
        s->capacidade = nova_capacidade;
    }

    // INSERCAO
    // se tem espaco cai pra add nota cai aq direto
    s->v_notas[s->total_notas] = nota;
    s->total_notas++;
}

void move_cursor_direita(Sistema *s)
{
    if (s->cursor.x + 5 < 1050)
        s->cursor.x += 5;
}
void move_cursor_esquerda(Sistema *s)
{
    if (s->cursor.x - 5 > 0)
        s->cursor.x -= 5;
}
void move_cursor_cima(Sistema *s)
{
    if (s->cursor.y - 5 > 0)
        s->cursor.y -= 5;
}
void move_cursor_baixo(Sistema *s)
{
    if (s->cursor.y + 5 < 600)
        s->cursor.y += 5;
}
void move_corrente_e_cursor_direita(Sistema *s)
{
    if (s->cursor.x + 5 < 1050 && s->nota_corrente >= 0)
    {
        s->v_notas[s->nota_corrente].tamanho.p.x += 5;
        s->cursor.x += 5;
    }
}
void move_corrente_e_cursor_esquerda(Sistema *s)
{
    if (s->cursor.x - 5 > 0 && s->nota_corrente >= 0)
    {
        s->v_notas[s->nota_corrente].tamanho.p.x -= 5;
        s->cursor.x -= 5;
    }
}
void move_corrente_e_cursor_cima(Sistema *s)
{
    if (s->cursor.y - 5 > 0 && s->nota_corrente >= 0)
    {
        s->v_notas[s->nota_corrente].tamanho.p.y -= 5;
        s->cursor.y -= 5;
    }
}
void move_corrente_e_cursor_baixo(Sistema *s)
{
    if (s->cursor.y + 5 < 600 && s->nota_corrente >= 0)
    {
        s->v_notas[s->nota_corrente].tamanho.p.y += 5;
        s->cursor.y += 5;
    }
}

void aumenta_nota_cima(Sistema *s)
{
    // n precisa calculo de limmite nessa
    if ((s->v_notas[s->nota_corrente].tamanho.p.y - 1) > 0)
    {
        s->v_notas[s->nota_corrente].tamanho.p.y -= 1;
        s->v_notas[s->nota_corrente].tamanho.altura += 1;
    }
}

void aumenta_nota_baixo(Sistema *s)
{
    int limite = 599 - s->v_notas[s->nota_corrente].tamanho.p.y; // calc limite embaixo
    if ((s->v_notas[s->nota_corrente].tamanho.altura) < limite)
    {
        s->v_notas[s->nota_corrente].tamanho.altura += 1;
    }
}

void aumenta_nota_direita(Sistema *s)
{
    int limite = 1049 - s->v_notas[s->nota_corrente].tamanho.p.x; // calc limite direita
    if ((s->v_notas[s->nota_corrente].tamanho.largura) < limite)
    {
        s->v_notas[s->nota_corrente].tamanho.largura += 1;
    }
}

void aumenta_nota_esquerda(Sistema *s)
{
    // n precisa calculo de limmite nessa
    if ((s->v_notas[s->nota_corrente].tamanho.p.x - 1) > 0)
    {
        s->v_notas[s->nota_corrente].tamanho.p.x -= 1;
        s->v_notas[s->nota_corrente].tamanho.largura += 1;
    }
}

void diminui_nota_cima(Sistema *s)
{
    if (s->v_notas[s->nota_corrente].tamanho.altura > 60)
    {
        s->v_notas[s->nota_corrente].tamanho.altura -= 1;
        if (!cursor_ta_na_nota(&s->v_notas[s->nota_corrente], &s->cursor))
            if (s->cursor.y - 1 > 0)
                s->cursor.y -= 1;
    }
}

void diminui_nota_baixo(Sistema *s)
{
    if (s->v_notas[s->nota_corrente].tamanho.altura > 60)
    {
        s->v_notas[s->nota_corrente].tamanho.p.y += 1;
        s->v_notas[s->nota_corrente].tamanho.altura -= 1;
        if (!cursor_ta_na_nota(&s->v_notas[s->nota_corrente], &s->cursor))
            if (s->cursor.y + 1 < 600)
                s->cursor.y += 1;
    }
}

void diminui_nota_direita(Sistema *s)
{
    if (s->v_notas[s->nota_corrente].tamanho.largura > 45)
    {
        s->v_notas[s->nota_corrente].tamanho.p.x += 1;
        s->v_notas[s->nota_corrente].tamanho.largura -= 1;
        if (!cursor_ta_na_nota(&s->v_notas[s->nota_corrente], &s->cursor))
            if (s->cursor.x + 1 < 1050)
                s->cursor.x += 1;
    }
}

void diminui_nota_esquerda(Sistema *s)
{
    if (s->v_notas[s->nota_corrente].tamanho.largura > 75)
    {
        s->v_notas[s->nota_corrente].tamanho.largura -= 1;
        if (!cursor_ta_na_nota(&s->v_notas[s->nota_corrente], &s->cursor))
            if (s->cursor.x - 1 > 0)
                s->cursor.x -= 1;
    }
}

void cursor_nota_final_vetor(Sistema *s)
{
    if (s->total_notas > 0)
    {
        s->cursor.x = s->v_notas[s->total_notas - 1].tamanho.p.x + 3;
        s->cursor.y = s->v_notas[s->total_notas - 1].tamanho.p.y + 3;
    }
}

// MESMO QUE O CURSOR SEJA PARA AS OUTRAS TELAS, PRECISA ESTAR AQUI A FUNCAO POIS AO ENTRAR NO MODO PRECISA ATUALIZAR A POSICAO DO CURSOR
void atualiza_cursor_editando(Sistema *s, char *texto)
{
    int linha = s->caractere_atual_editando / 100;

    char temp[101];
    int x = 0;

    int inicio_linha = linha * 100;

    for (int i = inicio_linha; i < s->caractere_atual_editando; i++)
    {
        temp[x++] = texto[i];
    }

    temp[x] = '\0';

    retangulo_t pos_cur = j_texto_contorno(temp);

    s->cursor_editando_inicio.x = 100 + pos_cur.tamanho.largura;
    s->cursor_editando_fim.x = s->cursor_editando_inicio.x;

    s->cursor_editando_inicio.y = 85 + linha * 20;
    s->cursor_editando_fim.y = 105 + linha * 20;
}

int principal(Sistema *s)
{
    retangulo_t tela_fundo;
    tela_fundo.inicio.x = 0;
    tela_fundo.inicio.y = 0;
    tela_fundo.tamanho.largura = 1050;
    tela_fundo.tamanho.altura = 600;

    cor_t cor_fundo;
    cor_fundo.vermelho = 0.15;
    cor_fundo.verde = 0.15;
    cor_fundo.azul = 0.15;
    cor_fundo.opacidade = 1.0;

    j_retangulo(tela_fundo, 0, cor_fundo, cor_fundo);

    s->nota_corrente = -1;
    for (int i = s->total_notas - 1; i >= 0; i--)
    {
        if (s->texto_busca[0] != '\0' &&
            strstr(s->v_notas[i].texto, s->texto_busca) == NULL &&
            strstr(s->v_notas[i].etiqueta, s->texto_busca) == NULL)
            continue;

        if (cursor_ta_na_nota(&s->v_notas[i], &s->cursor))
        {
            s->nota_corrente = i;
            break;
        }
    }

    for (int i = 0; i < s->total_notas; i++)
    {
        if (s->texto_busca[0] != '\0' &&
            strstr(s->v_notas[i].texto, s->texto_busca) == NULL &&
            strstr(s->v_notas[i].etiqueta, s->texto_busca) == NULL)
            continue;

        desenha_nota(&s->v_notas[i], s->nota_corrente, i);
    }

    desenha_cursor(&s->cursor);

    j_mostra();

    if (j_tem_tecla())
    {
        tecla_t tecla = j_tecla();

        if (tecla == T_ESC)
        {
            grava_nota_arq(s->v_notas, s->total_notas);
            return 0;
        }
        else if (tecla == 'f')
        {
            move_corrente_fim(s);
        }

        else if (tecla == T_DEL || tecla == 'd')
            remove_nota_corrente(s);
        else if (tecla == 'I')
            reinsere_nota_removida(s);
        else if (tecla == 'n')
            cria_nota_padrao(s);
        else if (tecla == 'g')
            grava_nota_arq(s->v_notas, s->total_notas);
        else if (tecla == 'e')
        {
            if (s->nota_corrente < 0)
            {
                return 1;
            }
            s->caractere_atual_editando = 0;
            strcpy(s->copia_texto, s->v_notas[s->nota_corrente].texto);
            atualiza_cursor_editando(s, s->v_notas[s->nota_corrente].texto);
            return 2;
        }
        else if ((tecla == T_DIREITA && j_shift()) || tecla == 'L')
            move_corrente_e_cursor_direita(s);
        else if ((tecla == T_ESQUERDA && j_shift()) || tecla == 'H')
            move_corrente_e_cursor_esquerda(s);
        else if ((tecla == T_CIMA && j_shift()) || tecla == 'K')
            move_corrente_e_cursor_cima(s);
        else if ((tecla == T_BAIXO && j_shift()) || tecla == 'J')
            move_corrente_e_cursor_baixo(s);
        else if ((tecla == T_DIREITA || tecla == 'l') && j_control())
            aumenta_nota_direita(s);
        else if ((tecla == T_ESQUERDA || tecla == 'h') && j_control())
            aumenta_nota_esquerda(s);
        else if ((tecla == T_CIMA || tecla == 'k') && j_control())
            aumenta_nota_cima(s);
        else if ((tecla == T_BAIXO || tecla == 'j') && j_control())
            aumenta_nota_baixo(s);
        else if ((tecla == T_DIREITA && j_alt()) || (tecla == 'o' && j_control()))
            diminui_nota_direita(s);
        else if ((tecla == T_ESQUERDA && j_alt()) || (tecla == 'y' && j_control()))
            diminui_nota_esquerda(s);
        else if ((tecla == T_CIMA && j_alt()) || (tecla == 'i' && j_control()))
            diminui_nota_cima(s);
        else if ((tecla == T_BAIXO && j_alt()) || (tecla == 'u' && j_control()))
            diminui_nota_baixo(s);
        else if (tecla == T_DIREITA || tecla == 'l')
            move_cursor_direita(s);
        else if (tecla == T_ESQUERDA || tecla == 'h')
            move_cursor_esquerda(s);
        else if (tecla == T_CIMA || tecla == 'k')
            move_cursor_cima(s);
        else if (tecla == T_BAIXO || tecla == 'j')
            move_cursor_baixo(s);
        else if (tecla == 'p')
            cursor_nota_final_vetor(s);
        else if (tecla == 't')
        {
            if (s->nota_corrente >= 0)
            {
                strcpy(s->copia_etiqueta, s->v_notas[s->nota_corrente].etiqueta);
                s->caractere_atual_editando = strlen(s->v_notas[s->nota_corrente].etiqueta);
                j_seleciona_fonte(NULL, 16);
                atualiza_cursor_editando(s, s->copia_etiqueta);
                return 3;
            }
        }
        else if (tecla == 'c')
        {
            if (s->nota_corrente >= 0)
            {
                s->copia_cor = s->v_notas[s->nota_corrente].cor;
                s->cor_atual_editando = 1;
                s->digito_atual_cor = 0;
                return 4;
            }
        }
        else if (tecla == 'b')
        {
            s->caractere_atual_editando = strlen(s->texto_busca);
            j_seleciona_fonte(NULL, 16);
            atualiza_cursor_editando(s, s->texto_busca);
            return 5;
        }
        else if (tecla == 'i')
        {
            move_corrente_inicio(s);
        }
    }

    j_cochila(SEGUNDOS_POR_QUADRO);
    return 1; // continua no mesmo estado
}

// MODO EDICAO
//  funcoes modo de edicao
void desenha_cursor_editando(Sistema *s)
{

    cor_t cor_cursor_editando;
    cor_cursor_editando.vermelho = 0;
    cor_cursor_editando.verde = 1;
    cor_cursor_editando.azul = 1;
    cor_cursor_editando.opacidade = 1;

    j_linha(s->cursor_editando_inicio, s->cursor_editando_fim, 1, cor_cursor_editando);
}

void move_cursor_editando_direita(Sistema *s, char *texto)
{
    if (s->caractere_atual_editando >= strlen(texto))
        return;

    s->caractere_atual_editando++;

    atualiza_cursor_editando(s, texto);
}

void move_cursor_editando_esquerda(Sistema *s, char *texto)
{
    if (s->caractere_atual_editando <= 0)
        return;

    s->caractere_atual_editando--;

    atualiza_cursor_editando(s, texto);
}

void remove_caractere_anterior(Sistema *s)
{
    if (s->caractere_atual_editando <= 0)
        return;

    s->caractere_atual_editando--;

    for (int i = s->caractere_atual_editando;
         s->copia_texto[i] != '\0';
         i++)
    {
        s->copia_texto[i] = s->copia_texto[i + 1];
    }

    atualiza_cursor_editando(s, s->copia_texto);
}

void remove_caractere_atual(Sistema *s)
{
    if (s->copia_texto[s->caractere_atual_editando] == '\0')
        return;

    for (int i = s->caractere_atual_editando;
         s->copia_texto[i] != '\0';
         i++)
    {
        s->copia_texto[i] = s->copia_texto[i + 1];
    }

    atualiza_cursor_editando(s, s->copia_texto);
}

void add_caractere(Sistema *s, tecla_t tecla)
{
    int tamanho = strlen(s->copia_texto);

    if (tamanho >= 499)
        return;

    for (int i = tamanho; i >= s->caractere_atual_editando; i--)
    {
        s->copia_texto[i + 1] = s->copia_texto[i];
    }

    s->copia_texto[s->caractere_atual_editando] = tecla;

    s->caractere_atual_editando++;

    atualiza_cursor_editando(s, s->copia_texto);
}

int edicao_texto(Sistema *s)
{

    // tela fundo
    retangulo_t tela_fundo;
    tela_fundo.inicio.x = 0;
    tela_fundo.inicio.y = 0;
    tela_fundo.tamanho.largura = 1050;
    tela_fundo.tamanho.altura = 600;

    cor_t cor_fundo;
    cor_fundo.vermelho = 0.15;
    cor_fundo.verde = 0.15;
    cor_fundo.azul = 0.15;
    cor_fundo.opacidade = 1;

    j_retangulo(tela_fundo, 0, cor_fundo, cor_fundo);

    // escrever texto que vai ser editado
    j_seleciona_fonte(NULL, 16);

    ponto_t posicao_texto_editando;
    posicao_texto_editando.x = 100;
    posicao_texto_editando.y = 100;

    cor_t cor_texto_editando;
    cor_texto_editando.vermelho = 1;
    cor_texto_editando.verde = 1;
    cor_texto_editando.azul = 1;
    cor_texto_editando.opacidade = 1;

    // quebra de linha
    char linha[100];
    linha[0] = '\0';
    int x = 0;
    for (int i = 0; s->copia_texto[i] != '\0'; i++)
    {
        if (x == 99)
        {
            j_texto(posicao_texto_editando, cor_texto_editando, linha);
            linha[0] = '\0';
            posicao_texto_editando.y += 20;
            x = 0;
        }
        linha[x] = s->copia_texto[i];
        x++;

        // p evitar dar erros
        linha[x] = '\0';
    }
    // precisa imprimir a ultima linha tbm (o que sobra)
    j_texto(posicao_texto_editando, cor_texto_editando, linha);

    // pensei em fazer um timer para o cursor piscar
    double tempo = j_relogio();
    if (((int)(tempo * 2)) % 2 == 0)
    {
        desenha_cursor_editando(s);
    }

    j_mostra();

    if (j_tem_tecla())
    {
        tecla_t tecla = j_tecla();

        if (tecla == T_ESC || (tecla == 'c' && j_control()))
            return 1;
        else if (tecla == T_ENTER)
        {
            strcpy(s->v_notas[s->nota_corrente].texto, s->copia_texto);
            return 1;
        }

        else if (tecla == T_DIREITA || (tecla == 'l' && j_control()))
            move_cursor_editando_direita(s, s->copia_texto);
        else if (tecla == T_ESQUERDA || (tecla == 'h' && j_control()))
            move_cursor_editando_esquerda(s, s->copia_texto);
        else if (tecla == T_BACKSPACE || (tecla == 'b' && j_control()))
            remove_caractere_anterior(s);
        else if (tecla == T_DEL || (tecla == 'd' && j_control()))
            remove_caractere_atual(s);
        else if (tecla >= 32 && tecla <= 126)
            add_caractere(s, tecla);
    }

    j_cochila(SEGUNDOS_POR_QUADRO);
    return 2;
}

// MODO EDICAO ETIQUETA

// reutilizei algumas funcoes da edicao texto outras preferi usar como base e alterar so o necessario

void remove_caractere_anterior_etiqueta(Sistema *s)
{
    if (s->caractere_atual_editando <= 0)
        return;

    s->caractere_atual_editando--;

    for (int i = s->caractere_atual_editando;
         s->copia_etiqueta[i] != '\0';
         i++)
    {
        s->copia_etiqueta[i] = s->copia_etiqueta[i + 1];
    }

    atualiza_cursor_editando(s, s->copia_etiqueta);
}

void remove_caractere_atual_etiqueta(Sistema *s)
{
    if (s->copia_etiqueta[s->caractere_atual_editando] == '\0')
        return;

    for (int i = s->caractere_atual_editando;
         s->copia_etiqueta[i] != '\0';
         i++)
    {
        s->copia_etiqueta[i] = s->copia_etiqueta[i + 1];
    }

    atualiza_cursor_editando(s, s->copia_etiqueta);
}

void add_caractere_editando(Sistema *s, tecla_t tecla)
{
    int tamanho = strlen(s->copia_etiqueta);

    if (tamanho >= 3)
        return;

    for (int i = tamanho; i >= s->caractere_atual_editando; i--)
    {
        s->copia_etiqueta[i + 1] = s->copia_etiqueta[i];
    }

    s->copia_etiqueta[s->caractere_atual_editando] = tecla;

    s->caractere_atual_editando++;

    atualiza_cursor_editando(s, s->copia_etiqueta);
}

void add_etiqueta_todas_notas(Sistema *s)
{
    for (int i = 0; i < s->total_notas; i++)
    {
        strcpy(s->v_notas[i].etiqueta, s->copia_etiqueta);
    }
}

int edicao_etiqueta(Sistema *s)
{
    // tela fundo
    retangulo_t tela_fundo;
    tela_fundo.inicio.x = 0;
    tela_fundo.inicio.y = 0;
    tela_fundo.tamanho.largura = 1050;
    tela_fundo.tamanho.altura = 600;

    cor_t cor_fundo;
    cor_fundo.vermelho = 0.15;
    cor_fundo.verde = 0.15;
    cor_fundo.azul = 0.15;
    cor_fundo.opacidade = 1;

    j_retangulo(tela_fundo, 0, cor_fundo, cor_fundo);

    // escrever texto que vai ser editado
    j_seleciona_fonte(NULL, 16);

    ponto_t posicao_etiqueta_editando;
    posicao_etiqueta_editando.x = 100;
    posicao_etiqueta_editando.y = 100;

    cor_t cor_etiqueta_editando;
    cor_etiqueta_editando.vermelho = 1;
    cor_etiqueta_editando.verde = 1;
    cor_etiqueta_editando.azul = 1;
    cor_etiqueta_editando.opacidade = 1;

    j_texto(posicao_etiqueta_editando, cor_etiqueta_editando, s->copia_etiqueta);

    // pensei em fazer um timer para o cursor piscar
    double tempo = j_relogio();
    if (((int)(tempo * 2)) % 2 == 0)
    {
        desenha_cursor_editando(s);
    }

    j_mostra();

    if (j_tem_tecla())
    {
        tecla_t tecla = j_tecla();

        if (tecla == T_ESC || (tecla == 'c' && j_control()))
            return 1;
        else if ((tecla == T_ENTER && j_shift()) || (tecla == 't' && j_control()))
        {
            if (strlen(s->copia_etiqueta) == 3)
            {
                add_etiqueta_todas_notas(s);
                return 1;
            }
        }
        else if (tecla == T_ENTER)
        {
            if (strlen(s->copia_etiqueta) == 3)
            {
                strcpy(s->v_notas[s->nota_corrente].etiqueta, s->copia_etiqueta);
                return 1;
            }
        }
        else if (tecla == T_DIREITA)
            move_cursor_editando_direita(s, s->copia_etiqueta);
        else if (tecla == T_ESQUERDA)
            move_cursor_editando_esquerda(s, s->copia_etiqueta);
        else if (tecla == T_BACKSPACE || (tecla == 'b' && j_control()))
            remove_caractere_anterior_etiqueta(s);
        else if (tecla == T_DEL || (tecla == 'd' && j_control())) // n pedia mas coloquei igual
            remove_caractere_atual_etiqueta(s);
        else if ((tecla >= '0' && tecla <= '9') || (tecla >= 'A' && tecla <= 'Z'))
            add_caractere_editando(s, tecla);
    }

    j_cochila(SEGUNDOS_POR_QUADRO);
    return 3;
}

// MODO EDICAO COR

void aumenta_valor_cor(Sistema *s)
{
    if (s->cor_atual_editando == 1)
    {
        if (s->copia_cor.vermelho < 255)
        {
            s->copia_cor.vermelho++;
        }
    }
    else if (s->cor_atual_editando == 2)
    {
        if (s->copia_cor.verde < 255)
        {
            s->copia_cor.verde++;
        }
    }
    else if (s->cor_atual_editando == 3)
    {
        if (s->copia_cor.azul < 255)
        {
            s->copia_cor.azul++;
        }
    }
}

void diminui_valor_cor(Sistema *s)
{
    if (s->cor_atual_editando == 1)
    {
        if (s->copia_cor.vermelho > 0)
        {
            s->copia_cor.vermelho--;
        }
    }
    else if (s->cor_atual_editando == 2)
    {
        if (s->copia_cor.verde > 0)
        {
            s->copia_cor.verde--;
        }
    }
    else if (s->cor_atual_editando == 3)
    {
        if (s->copia_cor.azul > 0)
        {
            s->copia_cor.azul--;
        }
    }
}

void aumenta_muito_valor_cor(Sistema *s)
{
    if (s->cor_atual_editando == 1)
    {
        if (s->copia_cor.vermelho < 225)
        {
            s->copia_cor.vermelho += 30;
        }
        else
        {
            s->copia_cor.vermelho = 255;
        }
    }
    else if (s->cor_atual_editando == 2)
    {
        if (s->copia_cor.verde < 225)
        {
            s->copia_cor.verde += 30;
        }
        else
        {
            s->copia_cor.verde = 255;
        }
    }
    else if (s->cor_atual_editando == 3)
    {
        if (s->copia_cor.azul < 225)
        {
            s->copia_cor.azul += 30;
        }
        else
        {
            s->copia_cor.azul = 255;
        }
    }
}

void diminui_muito_valor_cor(Sistema *s)
{
    if (s->cor_atual_editando == 1)
    {
        if (s->copia_cor.vermelho > 30)
        {
            s->copia_cor.vermelho -= 30;
        }
        else
        {
            s->copia_cor.vermelho = 0;
        }
    }
    else if (s->cor_atual_editando == 2)
    {
        if (s->copia_cor.verde > 30)
        {
            s->copia_cor.verde -= 30;
        }
        else
        {
            s->copia_cor.verde = 0;
        }
    }
    else if (s->cor_atual_editando == 3)
    {
        if (s->copia_cor.azul > 30)
        {
            s->copia_cor.azul -= 30;
        }
        else
        {
            s->copia_cor.azul = 0;
        }
    }
}

void add_cor_todas_notas(Sistema *s)
{
    for (int i = 0; i < s->total_notas; i++)
    {
        s->v_notas[i].cor = s->copia_cor;
    }
}

void add_cor_com_digitos(Sistema *s, tecla_t tecla)
{
    if (s->cor_atual_editando == 1)
    {
        if (s->digito_atual_cor == 0)
        {
            s->copia_cor.vermelho = tecla;
            s->digito_atual_cor++;
        }
        else if (s->digito_atual_cor == 1)
        {
            s->copia_cor.vermelho *= 10;
            s->copia_cor.vermelho += tecla;
            s->digito_atual_cor++;
        }
        else if (s->digito_atual_cor == 2)
        {
            if ((s->copia_cor.vermelho * 10) + tecla <= 255)
            {
                s->copia_cor.vermelho = (s->copia_cor.vermelho * 10) + tecla;
                s->digito_atual_cor = 0;
            }
        }
    }
    else if (s->cor_atual_editando == 2)
    {
        if (s->digito_atual_cor == 0)
        {
            s->copia_cor.verde = tecla;
            s->digito_atual_cor++;
        }
        else if (s->digito_atual_cor == 1)
        {
            s->copia_cor.verde *= 10;
            s->copia_cor.verde += tecla;
            s->digito_atual_cor++;
        }
        else if (s->digito_atual_cor == 2)
        {
            if ((s->copia_cor.verde * 10) + tecla <= 255)
            {
                s->copia_cor.verde = (s->copia_cor.verde * 10) + tecla;
                s->digito_atual_cor = 0;
            }
        }
    }
    else if (s->cor_atual_editando == 3)
    {
        if (s->digito_atual_cor == 0)
        {
            s->copia_cor.azul = tecla;
            s->digito_atual_cor++;
        }
        else if (s->digito_atual_cor == 1)
        {
            s->copia_cor.azul *= 10;
            s->copia_cor.azul += tecla;
            s->digito_atual_cor++;
        }
        else if (s->digito_atual_cor == 2)
        {
            if ((s->copia_cor.azul * 10) + tecla <= 255)
            {
                s->copia_cor.azul = (s->copia_cor.azul * 10) + tecla;
                s->digito_atual_cor = 0;
            }
        }
    }
}

int edicao_cor(Sistema *s)
{
    // tela fundo
    retangulo_t tela_fundo;
    tela_fundo.inicio.x = 0;
    tela_fundo.inicio.y = 0;
    tela_fundo.tamanho.largura = 1050;
    tela_fundo.tamanho.altura = 600;

    cor_t cor_fundo;
    cor_fundo.vermelho = 0.15;
    cor_fundo.verde = 0.15;
    cor_fundo.azul = 0.15;
    cor_fundo.opacidade = 1;

    j_retangulo(tela_fundo, 0, cor_fundo, cor_fundo);

    // escrever texto que vai ser editado
    j_seleciona_fonte(NULL, 16);

    ponto_t posicao_corR_editando;
    posicao_corR_editando.x = 250;
    posicao_corR_editando.y = 200;

    ponto_t posicao_corG_editando;
    posicao_corG_editando.x = 500;
    posicao_corG_editando.y = 200;

    ponto_t posicao_corB_editando;
    posicao_corB_editando.x = 750;
    posicao_corB_editando.y = 200;

    cor_t cor_corR_editando;
    cor_corR_editando.vermelho = 1;
    cor_corR_editando.verde = 0;
    cor_corR_editando.azul = 0;
    cor_corR_editando.opacidade = 1;

    cor_t cor_corG_editando;
    cor_corG_editando.vermelho = 0;
    cor_corG_editando.verde = 1;
    cor_corG_editando.azul = 0;
    cor_corG_editando.opacidade = 1;

    cor_t cor_corB_editando;
    cor_corB_editando.vermelho = 0;
    cor_corB_editando.verde = 0;
    cor_corB_editando.azul = 1;
    cor_corB_editando.opacidade = 1;

    char texto_r[4];
    char texto_g[4];
    char texto_b[4];

    snprintf(texto_r, 4, "%.0f", s->copia_cor.vermelho);
    snprintf(texto_g, 4, "%.0f", s->copia_cor.verde);
    snprintf(texto_b, 4, "%.0f", s->copia_cor.azul);

    j_texto(posicao_corR_editando, cor_corR_editando, texto_r);
    j_texto(posicao_corG_editando, cor_corG_editando, texto_g);
    j_texto(posicao_corB_editando, cor_corB_editando, texto_b);

    // fiz um retangulo que mostra qual cor ta sendo editada
    retangulo_t retangulo_cor_selecionada;
    if (s->cor_atual_editando == 1)
    {
        retangulo_cor_selecionada.inicio.x = 240;
    }
    else if (s->cor_atual_editando == 2)
    {
        retangulo_cor_selecionada.inicio.x = 490;
    }

    else if (s->cor_atual_editando == 3)
    {
        retangulo_cor_selecionada.inicio.x = 740;
    }

    retangulo_cor_selecionada.inicio.y = 184;
    retangulo_cor_selecionada.tamanho.altura = 20;
    retangulo_cor_selecionada.tamanho.largura = 55;

    // cor desse retangulo
    cor_t cor_retangulo_cor_selecionada;
    cor_retangulo_cor_selecionada.vermelho = 1;
    cor_retangulo_cor_selecionada.verde = 1;
    cor_retangulo_cor_selecionada.azul = 1;
    cor_retangulo_cor_selecionada.opacidade = 0.25;

    j_retangulo(retangulo_cor_selecionada, 1, cor_retangulo_cor_selecionada, cor_retangulo_cor_selecionada);

    j_mostra();

    if (j_tem_tecla())
    {
        tecla_t tecla = j_tecla();

        if (tecla == T_ESC || (tecla == 'b' && j_control()))
            return 1;
        else if ((tecla == T_ENTER && j_shift()) || (tecla == 't' && j_control()))
        {
            add_cor_todas_notas(s);
            return 1;
        }
        else if (tecla == T_ENTER)
        {
            s->v_notas[s->nota_corrente].cor = s->copia_cor;
            return 1;
        }
        else if (tecla == 'r' || tecla == 'e')
        {
            s->cor_atual_editando = 1;
            s->digito_atual_cor = 0;
        }

        else if (tecla == 'g' || tecla == 'v')
        {
            s->cor_atual_editando = 2;
            s->digito_atual_cor = 0;
        }

        else if (tecla == 'b' || tecla == 'a')
        {
            s->cor_atual_editando = 3;
            s->digito_atual_cor = 0;
        }

        else if (tecla == T_ESQUERDA || tecla == 'h')
        {
            if (s->cor_atual_editando == 1)
            {
                s->cor_atual_editando = 3;
                s->digito_atual_cor = 0;
            }
            else
            {
                s->cor_atual_editando--;
                s->digito_atual_cor = 0;
            }
        }
        else if (tecla == T_DIREITA || tecla == 'l')
        {
            if (s->cor_atual_editando == 3)
            {
                s->cor_atual_editando = 1;
                s->digito_atual_cor = 0;
            }
            else
            {
                s->cor_atual_editando++;
                s->digito_atual_cor = 0;
            }
        }
        else if ((tecla == T_CIMA && j_shift()) || tecla == 'K')
            aumenta_muito_valor_cor(s);
        else if ((tecla == T_BAIXO && j_shift()) || tecla == 'J')
            diminui_muito_valor_cor(s);
        else if (tecla == T_CIMA || tecla == 'k')
            aumenta_valor_cor(s);
        else if (tecla == T_BAIXO || tecla == 'j')
            diminui_valor_cor(s);
        else if (tecla >= '0' && tecla <= '9')
            add_cor_com_digitos(s, tecla - '0');
    }

    j_cochila(SEGUNDOS_POR_QUADRO);
    return 4;
}

// MODO EDICAO BUSCA

void remove_caractere_anterior_busca(Sistema *s)
{
    if (s->caractere_atual_editando <= 0)
        return;

    s->caractere_atual_editando--;

    for (int i = s->caractere_atual_editando;
         s->texto_busca[i] != '\0';
         i++)
    {
        s->texto_busca[i] = s->texto_busca[i + 1];
    }

    atualiza_cursor_editando(s, s->texto_busca);
}

void remove_caractere_atual_busca(Sistema *s)
{
    if (s->texto_busca[s->caractere_atual_editando] == '\0')
        return;

    for (int i = s->caractere_atual_editando;
         s->texto_busca[i] != '\0';
         i++)
    {
        s->texto_busca[i] = s->texto_busca[i + 1];
    }

    atualiza_cursor_editando(s, s->texto_busca);
}

void add_caractere_busca(Sistema *s, tecla_t tecla)
{
    int tamanho = strlen(s->texto_busca);

    if (tamanho >= 499)
        return;

    for (int i = tamanho; i >= s->caractere_atual_editando; i--)
    {
        s->texto_busca[i + 1] = s->texto_busca[i];
    }

    s->texto_busca[s->caractere_atual_editando] = tecla;

    s->caractere_atual_editando++;

    atualiza_cursor_editando(s, s->texto_busca);
}

int edicao_busca(Sistema *s)
{
    // tela fundo
    retangulo_t tela_fundo;
    tela_fundo.inicio.x = 0;
    tela_fundo.inicio.y = 0;
    tela_fundo.tamanho.largura = 1050;
    tela_fundo.tamanho.altura = 600;

    cor_t cor_fundo;
    cor_fundo.vermelho = 0.15;
    cor_fundo.verde = 0.15;
    cor_fundo.azul = 0.15;
    cor_fundo.opacidade = 1;

    j_retangulo(tela_fundo, 0, cor_fundo, cor_fundo);

    // escrever texto de busca
    j_seleciona_fonte(NULL, 16);

    ponto_t posicao_busca_editando;
    posicao_busca_editando.x = 100;
    posicao_busca_editando.y = 100;

    cor_t cor_busca_editando;
    cor_busca_editando.vermelho = 1;
    cor_busca_editando.verde = 1;
    cor_busca_editando.azul = 1;
    cor_busca_editando.opacidade = 1;

    j_texto(posicao_busca_editando, cor_busca_editando, s->texto_busca);

    // pensei em fazer um timer para o cursor piscar
    double tempo = j_relogio();
    if (((int)(tempo * 2)) % 2 == 0)
    {
        desenha_cursor_editando(s);
    }

    j_mostra();

    if (j_tem_tecla())
    {
        tecla_t tecla = j_tecla();

        if (tecla == T_ESC || (tecla == 'c' && j_control()))
        {
            s->texto_busca[0] = '\0';
            s->caractere_atual_editando = 0;
            atualiza_cursor_editando(s, s->texto_busca);
            return 1;
        }
        else if (tecla == T_ENTER)
            return 1;
        else if (tecla == T_DIREITA || (tecla == 'l' && j_control()))
            move_cursor_editando_direita(s, s->texto_busca);
        else if (tecla == T_ESQUERDA || (tecla == 'h' && j_control()))
            move_cursor_editando_esquerda(s, s->texto_busca);
        else if (tecla == T_HOME || (tecla == 'k' && j_control()))
        {
            s->caractere_atual_editando = 0;
            atualiza_cursor_editando(s, s->texto_busca);
        }
        else if (tecla == T_END || (tecla == 'j' && j_control()))
        {
            s->caractere_atual_editando = strlen(s->texto_busca);
            atualiza_cursor_editando(s, s->texto_busca);
        }
        else if (tecla == T_BACKSPACE || (tecla == 'b' && j_control()))
            remove_caractere_anterior_busca(s);
        else if (tecla == T_DEL || (tecla == 'd' && j_control()))
            remove_caractere_atual_busca(s);
        else if ((tecla >= 'a' && tecla <= 'z') || (tecla >= 'A' && tecla <= 'Z') || (tecla >= '0' && tecla <= '9') || tecla == ' ')
            add_caractere_busca(s, tecla);
    }

    j_cochila(SEGUNDOS_POR_QUADRO);
    return 5;
}

int main()
{
    Sistema s;

    s.proximo_id = 1;
    s.capacidade = 10;
    s.total_notas = 0;
    s.nota_corrente = -1;
    s.existe_ultima_removida = false;
    s.cursor.x = 100;
    s.cursor.y = 100;
    s.cursor_editando_inicio.x = 100;
    s.cursor_editando_inicio.y = 85;
    s.cursor_editando_fim.x = 100;
    s.cursor_editando_fim.y = 105;

    s.caractere_atual_editando = 0;
    s.texto_busca[0] = '\0';

    s.cor_atual_editando = 1;
    s.digito_atual_cor = 0;

    s.v_notas = malloc(s.capacidade * sizeof(Nota));
    if (s.v_notas == NULL)
    {
        printf("Erro de memoria, experiemente fechar alguns programas.\n");
        return 1;
    }

    tamanho_t tamanho_janela;
    tamanho_janela.altura = 600;
    tamanho_janela.largura = 1050;

    char titulo[] = "Gerenciador de Notas - Tonny Rettore";

    j_inicializa(tamanho_janela, titulo);

    le_nota_arq(&s);

    int estado_atual = 1;
    while (estado_atual != 0)
    {
        if (estado_atual == 1)
        {
            estado_atual = principal(&s);
        }
        else if (estado_atual == 2)
        {
            estado_atual = edicao_texto(&s);
        }
        else if (estado_atual == 3)
        {
            estado_atual = edicao_etiqueta(&s);
        }
        else if (estado_atual == 4)
        {
            estado_atual = edicao_cor(&s);
        }
        else if (estado_atual == 5)
        {
            estado_atual = edicao_busca(&s);
        }
    }

    j_finaliza();
    free(s.v_notas);
}
