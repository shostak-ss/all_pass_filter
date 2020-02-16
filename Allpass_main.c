#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define Q31_1_BASE (1LL<<30)
#define Q31_1_MAX  ((1<<30)-1)
#define Q31_1_MIN  (-1<<30)
#define SIZE (128)
#define PI (3.14159265358979323846)

int32_t flt2fixd(double x)
{
	if (x >= 1)
		return Q31_1_MAX;
	else if (x < -1)
		return Q31_1_MIN;

	int32_t res = x * (double)Q31_1_BASE;
	return res;
}

float fixd2flt(int32_t x)
{
	float res = (float)(x) / ((float)Q31_1_BASE);
	return res;
}

typedef struct
{
	uint8_t ChunkID[4];
	uint32_t ChunkSize;
	uint8_t Format[4];
	uint8_t Subchunk1ID[4];
	uint32_t Subchunk1Size;
	uint16_t AudioFormat;
	uint16_t NumChannels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign;
	uint16_t BitsPreSample;

	uint8_t ID[4];
	uint32_t size;
} wav_header;

void wav_header_init(wav_header* header, int32_t sample_lenth) {
	header->ChunkID[0] = 'R';
	header->ChunkID[1] = 'I';
	header->ChunkID[2] = 'F';
	header->ChunkID[3] = 'F';
	header->Format[0] = 'W';
	header->Format[1] = 'A';
	header->Format[2] = 'V';
	header->Format[3] = 'E';
	header->Subchunk1ID[0] = 'f';
	header->Subchunk1ID[1] = 'm';
	header->Subchunk1ID[2] = 't';
	header->Subchunk1ID[3] = ' ';

	header->AudioFormat = 1; //pcm
	header->NumChannels = 2;
	header->SampleRate = 48000;
	header->BitsPreSample = 16;
	header->ByteRate = (header->SampleRate * header->BitsPreSample * header->NumChannels) / 8; //!
	header->BlockAlign = header->NumChannels * header->BitsPreSample / 8;
	header->Subchunk1Size = 16;
	header->size = sample_lenth * header->NumChannels * header->BitsPreSample / 8;
	header->ID[0] = 'd';
	header->ID[1] = 'a';
	header->ID[2] = 't';
	header->ID[3] = 'a';
	header->ChunkSize = 4 + (8 + header->Subchunk1Size) + (8 + header->size);
}

int16_t sweep_wave(float amplitude, float start_fq, float end_fq, int32_t sample_lenth, int t, int32_t fs)
{
	int32_t sample_sig;
	float omeg1 = 2 * PI * start_fq;
	float omeg2 = 2 * PI * end_fq;
	float tmp1 = log(omeg2 / omeg1);

	float n;
	float tmp2;
	n = (float)t / sample_lenth;
	tmp2 = exp(n * tmp1) - 1.0;
	sample_sig = flt2fixd(amplitude * sin(omeg1 * sample_lenth * tmp2 / (fs * tmp1)));

	return sample_sig >> 15;
}

void all_pass_coeffs(double Fc, double Fs, int32_t* coeffs)
{
	double wñ = 2 * Fc / Fs;
	coeffs[0] = flt2fixd((tan(PI*wñ/2)-1) / (tan(PI*wñ/2) + 1));
}

int16_t allpass_HP(int32_t* coeffs, int16_t* sample_buffer, int16_t signal)
{
	int32_t c = (int32_t)coeffs[0];
	int64_t x = ((int64_t)signal << 31) - c * sample_buffer[0];
	int16_t xh_new = (int16_t)(x >> 31);
	int64_t y = c * xh_new + ((int64_t)sample_buffer[0] << 31);
	sample_buffer[0] = xh_new;

	int16_t ap_y= (int16_t)(y >> 31);
	int32_t res = ((int32_t)signal - ap_y) >> 1; //change to plus for LP

	return (int16_t)res;
}


void main() {
	int32_t fs = 48000;
	float freq = 20;
	float end_freq = 20000;
	float amplitude = 0.8;
	int32_t sample_lenth = 0;
	float fc = 440.0;

	uint32_t second = 0;
	printf("Enter duration of record:");
	scanf_s("%d", &second);
	sample_lenth = second * fs;
	printf("Amount of samples=%d\n", sample_lenth);

	FILE* file_out;
	wav_header header;
	wav_header_init(&header, sample_lenth);

	fopen_s(&file_out, "test_signal.wav", "wb");
	fwrite(&header, sizeof(header), 1, file_out);

	int16_t buffer[2 * SIZE];
	int16_t sample_buffer[2] = { 0, 0 };
	int32_t coeffs[2] = { 0, 0 };

	int16_t input_sample;
	int16_t output;

	all_pass_coeffs(fc, fs, coeffs);

	int j;
	for (int t = 0; t < sample_lenth;) {

		for (int n = 0; n < SIZE; n++, t++) {

			input_sample = sweep_wave(amplitude, freq, end_freq, sample_lenth, t, fs);

			output = allpass_HP(coeffs, sample_buffer, input_sample);

			buffer[2 * n] = output;
			buffer[2 * n + 1] = output;
			j = n;
		}

		fwrite(buffer, 2 * (j + 1) * sizeof(int16_t), 1, file_out);
	}

	fclose(file_out);
	printf("\n WAV File was generated.\n");
	system("pause");
}