#ifndef LZW_C_
#define LZW_C_

#include <stdio.h>
#include <stdlib.h>

#define byte unsigned char

#define LZW_MAX_TABLE_SIZE 4096
typedef struct LZW{
	int size;
	byte tamanhos[LZW_MAX_TABLE_SIZE];
	byte* valores[LZW_MAX_TABLE_SIZE];
}LZW;

void LZW_inserir(LZW* lzw, byte* valores, byte tamanho){
	lzw->tamanhos[lzw->size] = tamanho;
	lzw->valores[lzw->size++] = valores;
}

void LZW_iniciar(LZW* lzw, byte lzw_min_code_size){
	lzw->size = 0;
	
	for (int i = 0; i < 1<<lzw_min_code_size; i++)
	{
		byte* m = malloc(1);
		*m = i;
		LZW_inserir(lzw, m,1);
	}
	//insere vazios para as posicoes do clear_code e eoi_code
	LZW_inserir(lzw, 0, 0);
	LZW_inserir(lzw, 0, 0);
}

void LZW_liberar(LZW *lzw){
	for (int i = 0; i < lzw->size; i++)
		if(lzw->valores[i])
			free(lzw->valores[i]);
}

int LZW_procurar(LZW * lzw, byte* buffer, int buflen){
	for (int i = 0; i < lzw->size; i++)
		if(lzw->tamanhos[i] == buflen){
			for (int j = 0; j < buflen; j++)
				if(buffer[j] != lzw->valores[i][j])
				goto nop;
			return i;
nop:
		}
	return -1;	
}

int LZW_compress(byte* input, int input_len, byte* output, byte lzw_min_code_size){
	LZW lzw;
	
	LZW_iniciar(&lzw, lzw_min_code_size);
	int clear_code = 1<<lzw_min_code_size;
	int eoi_code = clear_code+1;
	
	byte* index_buffer = malloc(input_len);
	int index_buffer_size = 0;

	int output_len = 0;
	output[output_len++] = clear_code;

	index_buffer[index_buffer_size++] = input[0];

	for (int i = 1; i < input_len; i++)
	{
		byte K = input[i];
		
		//cria o index_buffer + K
		index_buffer[index_buffer_size] = K;
		//procura o index_buffer + K
		int achado = LZW_procurar(&lzw, index_buffer, index_buffer_size+1);
		if(achado!=-1){
			//achou
			index_buffer_size++;
		}
		else{
			//não achou
			//copia para um novo
			byte *novo = malloc(index_buffer_size+1);
			for (int j = 0; j < index_buffer_size+1; j++)
				novo[j] = index_buffer[j];
			//insere na tabela				
			LZW_inserir(&lzw,novo,index_buffer_size+1);
			
			int achado = LZW_procurar(&lzw, index_buffer, index_buffer_size);

			output[output_len++] = achado;
			
			index_buffer[0] = K;
			index_buffer_size = 1;
		}
	}
	int achado = LZW_procurar(&lzw, index_buffer, index_buffer_size);
	output[output_len++] = achado;
	
	output[output_len++] = eoi_code;
	
	free(index_buffer);
	
	return output_len;
}

int LZW_decompress(byte* input, int input_len, byte* output, byte lzw_min_code_size){
	LZW lzw;
	
	LZW_iniciar(&lzw, lzw_min_code_size);
	int clear_code = 1<<lzw_min_code_size;
	int eoi_code = clear_code+1;

	int output_len = 0;
	byte code = input[1]; //pula primeiro code
	
	output[output_len++] = code;

	for (int i = 2; i < input_len-1; i++)
	{
		byte oldcode = code;
		code = input[i];

		if(code < lzw.size){
			// adiciona valor de code no output
			for (int j = 0; j < lzw.tamanhos[code]; j++)
				output[output_len++] = lzw.valores[code][j];
			byte k = lzw.valores[code][0];
			byte *novo = malloc(lzw.tamanhos[oldcode]+1);
			//copia de oldcode
			for (int j = 0; j < lzw.tamanhos[oldcode]; j++)
				novo[j] = lzw.valores[oldcode][j];
			novo[lzw.tamanhos[oldcode]] = k;
			LZW_inserir(&lzw,novo,lzw.tamanhos[oldcode]+1);
		}
		else{
			byte k = lzw.valores[oldcode][0];
			for (int j = 0; j < lzw.tamanhos[oldcode]; j++)
				output[output_len++] = lzw.valores[oldcode][j];
			output[output_len++] = k;
			
			byte *novo = malloc(lzw.tamanhos[oldcode]+1);
			//copia de oldcode
			for (int j = 0; j < lzw.tamanhos[oldcode]; j++)
				novo[j] = lzw.valores[oldcode][j];
			novo[lzw.tamanhos[oldcode]] = k;
			LZW_inserir(&lzw,novo,lzw.tamanhos[oldcode]+1);
		}

	}
	return output_len;
}

int LZW_decompress_bitstream_next(byte* input, int input_len, int* byte_index, int* bit_index, int bits){
    int cod_montado = 0;
    int len_cod_montado = 0;

	while (*byte_index<input_len)
	{
		while (*bit_index<8)
		{
			int bit = (input[*byte_index] >> *bit_index) & 1;
			cod_montado = ((cod_montado>>1) & ~(1 << 31)) | (bit << 31);
            len_cod_montado++;
			(*bit_index)++;
			if(len_cod_montado==bits){
                cod_montado = cod_montado>>(32-bits) & ((1<<bits) -1);
				return cod_montado;
			}
		}
		
		(*byte_index)++;
		*bit_index = 0;
	}
	return 0;
}

int LZW_decompress_from_bitstream(byte* input, int input_len, byte* output, byte lzw_min_code_size){
	byte example_data[] = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2,2,2,2,0,0,0,0,1,1,1,2,2,2,0,0,0,0,1,1,1, 2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1};
	byte example_compressed[100];
	byte example_decompressed[100];

	int example_output_len = LZW_compress(example_data,100,example_compressed,2);
	
	LZW lzw;
	
	LZW_iniciar(&lzw, lzw_min_code_size);
	int clear_code = 1<<lzw_min_code_size;
	int eoi_code = clear_code+1;

	int output_len = 0;
	int bits = lzw_min_code_size+1;
    int cod_montado = 0;
    int len_cod_montado = 0;

	int byte_index = 0;
	int bit_index = 0;

	//pula o primeiro
	byte code = LZW_decompress_bitstream_next(input, input_len,&byte_index,&bit_index, bits);
	int code_index = 0;
	
	if(code == clear_code){
		code = LZW_decompress_bitstream_next(input, input_len,&byte_index,&bit_index, bits);
		code_index++;
	}
	// printf("#%u\t( #%u )\n",code, example_compressed[code_index++]);
	output[output_len++] = code;
	// printf("%u\t( %u )\n",output[output_len-1], example_data[output_len-1]);

	while (byte_index < input_len)
	{
		byte oldcode = code;
		code = LZW_decompress_bitstream_next(input, input_len,&byte_index,&bit_index, bits);
		if(code == eoi_code)
			break;
		// printf("#%u\t( #%u )\n",code, example_compressed[code_index++]);
		if(code == clear_code){
			bits = lzw_min_code_size+1;
			code = oldcode;
		}
		else if(code < lzw.size){
			// adiciona valor de code no output
			for (int j = 0; j < lzw.tamanhos[code]; j++){
				output[output_len++] = lzw.valores[code][j];
				// printf("%u\t( %u )\n",output[output_len-1], example_data[output_len-1]);
			}
			byte k = lzw.valores[code][0];
			byte *novo = malloc(lzw.tamanhos[oldcode]+1);
			//copia de oldcode
			for (int j = 0; j < lzw.tamanhos[oldcode]; j++)
				novo[j] = lzw.valores[oldcode][j];
			novo[lzw.tamanhos[oldcode]] = k;
			
			if(lzw.size == (1<<bits)-1)
				bits++;
			LZW_inserir(&lzw,novo,lzw.tamanhos[oldcode]+1);
		}
		else{
			byte k = lzw.valores[oldcode][0];
			for (int j = 0; j < lzw.tamanhos[oldcode]; j++){
				output[output_len++] = lzw.valores[oldcode][j];
				// printf("%u\t( %u )\n",output[output_len-1], example_data[output_len-1]);
			}
			output[output_len++] = k;
			// printf("%u\t( %u )\n",output[output_len-1], example_data[output_len-1]);
			
			byte *novo = malloc(lzw.tamanhos[oldcode]+1);
			//copia de oldcode
			for (int j = 0; j < lzw.tamanhos[oldcode]; j++)
				novo[j] = lzw.valores[oldcode][j];
			novo[lzw.tamanhos[oldcode]] = k;
			if(lzw.size == (1<<bits)-1)
				bits++;
			LZW_inserir(&lzw,novo,lzw.tamanhos[oldcode]+1);
		}
	}
	
	return output_len;
}
// void main(){
// 	byte data[] = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2,2,2,2,0,0,0,0,1,1,1,2,2,2,0,0,0,0,1,1,1, 2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1};
// 	byte compressed[100];
// 	byte decompressed[100];

// 	int output_len = compress(data,100,compressed,2);
// 	for (int i = 0; i < output_len; i++)
// 		printf("%u ", compressed[i]);
	
// 	puts("\n");
	
// 	output_len = decompress(compressed,output_len,decompressed,2);
// 	for (int i = 0; i < output_len; i++)
// 		printf("%u ", decompressed[i]);
	
// }
#endif // LZW_C_