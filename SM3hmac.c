#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<unistd.h>

//#pragma warning(disable : 4996)// ����΢��VS��׼ // Linux��ע�͵�����

typedef struct _MsgInt {
	unsigned int* msgInt;//ԭʼ��Ϣ�ַ�ת��Ϊintָ�����飬ÿ4���ַ�ת��Ϊһ��int����
	int intCount;//int������
}MsgInt;

typedef struct _ExtendMsgInt {
	unsigned int W[68];
	unsigned int W1[64];
}ExtendMsgInt;

/*
 * �꺯��NOT_BIG_ENDIAN()
 * ���ڲ������л����Ƿ�Ϊ��ˣ�С�˷���true
 */
static const int endianTestNum = 1;
#define NOT_BIG_ENDIAN() ( *(char *)&endianTestNum == 1 )

#define FF_LOW(x,y,z) ( (x) ^ (y) ^ (z))
#define FF_HIGH(x,y,z) (((x) & (y)) | ( (x) & (z)) | ( (y) & (z)))

#define GG_LOW(x,y,z) ( (x) ^ (y) ^ (z))
#define GG_HIGH(x,y,z) (((x) & (y)) | ( (~(x)) & (z)) )

#define ROTATE_LEFT(uint32,shift) ( ( ( (uint32) << (shift) ) | ( (uint32) >> (32 - (shift)) ) ) )

#define P0(x) ((x) ^  ROTATE_LEFT((x),9) ^ ROTATE_LEFT((x),17))
#define P1(x) ((x) ^  ROTATE_LEFT((x),15) ^ ROTATE_LEFT((x),23))

/*
 * �꺯��UCHAR_2_UINT(uchr8,uint32,i,notBigendian)
 * uchr8        -- unsigned char - 8bit
 * uint32       -- unsigned int  - 32bit
 * i            -- int
 * notBigendian -- int/bool
 * ��uchr8���յ��ַ�����,ת���ɴ�˱�ʾ��uint32���ӵײ㿴�����ư����λ�ҵ�λ���У�
 * NOT_BIG_ENDIAN()���黷�����Ǵ��ʱnotBigendianΪ�棬���ô˺꺯��
 */
#define UCHAR_2_UINT(uchr8,uint32,i,notBigendian)				\
{																\
	if(notBigendian){                                           \
		(uint32) = ((unsigned int) (uchr8)[(i)    ] << 24 )		\
				 | ((unsigned int) (uchr8)[(i) + 1] << 16 )		\
				 | ((unsigned int) (uchr8)[(i) + 2] << 8  )		\
				 | ((unsigned int) (uchr8)[(i) + 3]       );	\
	}															\
}

 /*
  * �꺯��UINT_2_UCHAR(uint32,uchr8,i,notBigendian)
  * uchr8        -- unsigned char - 8bit
  * uint32       -- unsigned int  - 32bit
  * i            -- int
  * notBigendian -- int/bool
  * ����˱�ʾ��uint32,ת����uchr8���ַ�����
  * NOT_BIG_ENDIAN()���黷�����Ǵ��ʱnotBigendianΪ�棬���ô˺꺯��
  */
#define UINT_2_UCHAR(uint32,uchr8,i,notBigendian)				\
{																\
	if(notBigendian){                                           \
		(uchr8)[(i)    ] = (unsigned char)((uint32) >> 24);		\
		(uchr8)[(i) + 1] = (unsigned char)((uint32) >> 16);		\
		(uchr8)[(i) + 2] = (unsigned char)((uint32) >> 8 );		\
		(uchr8)[(i) + 3] = (unsigned char)((uint32)      );		\
	}															\
}

MsgInt MsgFill512(unsigned char* msg, int notBigendian)
{
	MsgInt filledMsgInt;
	unsigned long long msgLength = strlen((char*)msg);// ��������и�ǿ��ת����strlen��֧��unsigned char*
	unsigned long long msgbitLength = msgLength * 8; // ��ԭʼ��Ϣ�ı��س���
	int zeroFill = 448 - (msgbitLength + 8) % 512; // +8�ǲ���0x80=0b1000_0000
	unsigned char* zeroChar = (unsigned char*)malloc(zeroFill / 8);

	memset(zeroChar, 0, zeroFill / 8);

	int totalChrLength = msgLength + 1 + zeroFill / 8 + 8;

	filledMsgInt.msgInt = (unsigned int*)malloc(totalChrLength / 4);
	filledMsgInt.intCount = totalChrLength / 4;//totalChrLength���ַ�����8bit/����msgIntΪ32bit/��

	unsigned char* msgFill = (unsigned char*)malloc(totalChrLength);// 1��ʾ0x80�ĳ��ȣ�һ���ֽ�
	memcpy(msgFill, msg, msgLength);
	unsigned char one = 0x80;
	memcpy(msgFill + msgLength, &one, 1);
	memcpy(msgFill + msgLength + 1, zeroChar, zeroFill / 8);

	unsigned char msgLenChr[8];
	if (notBigendian) { // С��ϵͳ��long long �������ڴ��еߵ��洢�ģ�������Ҫת��
		for (int i = 0; i < 8; i++) {
			msgLenChr[i] = msgbitLength >> (56 - 8 * i);
		}
		memcpy(msgFill + msgLength + 1 + zeroFill / 8, msgLenChr, 8);
	}
	else { // ����Ǵ��ϵͳ��ֱ�ӿ���msgbitLength�ڴ����ݼ���
		memcpy(msgFill + msgLength + 1 + zeroFill / 8, &msgbitLength, 8);
	}

	for (int i = 0; i < filledMsgInt.intCount; i++) {
		unsigned char msgSlice[4] = { *(msgFill + i * 4),*(msgFill + i * 4 + 1),*(msgFill + i * 4 + 2),*(msgFill + i * 4 + 3) };
		UCHAR_2_UINT(msgSlice, filledMsgInt.msgInt[i], 0, notBigendian);
	}

	return filledMsgInt;
}

ExtendMsgInt MsgExtend(unsigned int msgInt16[])
{
	ExtendMsgInt etdMsgInt;

	/*for (int i = 0; i < 16; i++) {
		etdMsgInt.W[i] = msgInt16[i];
	}*/
	etdMsgInt.W[0] = msgInt16[0]; // ���Сѭ��
	etdMsgInt.W[1] = msgInt16[1];
	etdMsgInt.W[2] = msgInt16[2];
	etdMsgInt.W[3] = msgInt16[3];
	etdMsgInt.W[4] = msgInt16[4];
	etdMsgInt.W[5] = msgInt16[5];
	etdMsgInt.W[6] = msgInt16[6];
	etdMsgInt.W[7] = msgInt16[7];
	etdMsgInt.W[8] = msgInt16[8];
	etdMsgInt.W[9] = msgInt16[9];
	etdMsgInt.W[10] = msgInt16[10];
	etdMsgInt.W[11] = msgInt16[11];
	etdMsgInt.W[12] = msgInt16[12];
	etdMsgInt.W[13] = msgInt16[13];
	etdMsgInt.W[14] = msgInt16[14];
	etdMsgInt.W[15] = msgInt16[15];

	register unsigned int tmp;
	for (int j = 16; j < 68; j++) {
		tmp = etdMsgInt.W[j - 16] ^ etdMsgInt.W[j - 9] ^ ROTATE_LEFT(etdMsgInt.W[j - 3], 15);
		etdMsgInt.W[j] = P1(tmp) ^ ROTATE_LEFT(etdMsgInt.W[j - 13], 7) ^ etdMsgInt.W[j - 6];
	}
	for (int j = 0; j < 64; j++) {
		etdMsgInt.W1[j] = etdMsgInt.W[j] ^ etdMsgInt.W[j + 4];
	}
	return etdMsgInt;
}

void CF(unsigned int Vi[], unsigned int msgInt16[], unsigned int W[], unsigned int W1[])
{
	//unsigned int regA2H[8]; // A~H 8���Ĵ���
	register unsigned int A, B, C, D, E, F, G, H;// A~H 8���Ĵ���
	register unsigned int SS1, SS2, TT1, TT2; // �м����

	/*for (int i = 0; i < 8; i++) {
		regA2H[i] = Vi[i];
	}*/
	A = Vi[0];// ���Сѭ��
	B = Vi[1];
	C = Vi[2];
	D = Vi[3];
	E = Vi[4];
	F = Vi[5];
	G = Vi[6];
	H = Vi[7];

	register unsigned int T = 0x79cc4519; // �ĵ��еĳ���Tj���˴���T
	for (int j = 0; j < 64; j++) {
		if (j >= 16) {
			T = 0x7a879d8a;
		}
		SS1 = ROTATE_LEFT(ROTATE_LEFT(A, 12) + E + ROTATE_LEFT(T, j), 7);
		SS2 = SS1 ^ ROTATE_LEFT(A, 12);
		if (j < 16) {
			TT1 = FF_LOW(A, B, C) + D + SS2 + W1[j];
			TT2 = GG_LOW(E, F, G) + H + SS1 + W[j];
		}
		else {
			TT1 = FF_HIGH(A, B, C) + D + SS2 + W1[j];
			TT2 = GG_HIGH(E, F, G) + H + SS1 + W[j];
		}
		D = C;
		C = ROTATE_LEFT(B, 9);
		B = A;
		A = TT1;
		H = G;
		G = ROTATE_LEFT(F, 19);
		F = E;
		E = P0(TT2);
	}
	//for (int i = 0; i < 8; i++) { // ���� ABCDEFH ^ Vi
	//	regA2H[i] ^= Vi[i];
	//	Vi[i] = regA2H[i];
	//}
	A ^= Vi[0];// ���Сѭ��
	B ^= Vi[1];
	C ^= Vi[2];
	D ^= Vi[3];
	E ^= Vi[4];
	F ^= Vi[5];
	G ^= Vi[6];
	H ^= Vi[7];

	Vi[0] = A;// ���Сѭ��
	Vi[1] = B;
	Vi[2] = C;
	Vi[3] = D;
	Vi[4] = E;
	Vi[5] = F;
	Vi[6] = G;
	Vi[7] = H;

}

void SM3Hash(unsigned char* msgText, int notBigendian, unsigned char sm3HashChr32[])
{
	MsgInt filledMsgInt = MsgFill512(msgText, notBigendian);
	// �����õ���Ϣ��512bit���з��飬��ÿ16��intһ��
	register int groupAmount = filledMsgInt.intCount / 16; // ����������

	unsigned int V[8] = {
	0x7380166F,0x4914B2B9,0x172442D7,0xDA8A0600,
	0xA96F30BC,0x163138AA,0xE38DEE4D,0xB0FB0E4E
	};

	for (int i = 0; i < groupAmount; i++) {
		unsigned int* bi = (i << 4) + filledMsgInt.msgInt;
		ExtendMsgInt etdMsgInt = MsgExtend(bi);
		CF(V, bi, etdMsgInt.W, etdMsgInt.W1); // ÿһ��ѹ������V
	}
	// ֱ�����int�͵��Ӵ�ֵ����
	/*for (int i = 0; i < 8; i++) {
		printf("%08x ", V[i]);
	}*/
	//return V;

	/*for (int i = 0; i < 8; i++) { // ���Ӵ�ֵ����char����
		UINT_2_UCHAR(V[i], sm3HashChr32, 4 * i, notBigendian);
	}*/

	UINT_2_UCHAR(V[0], sm3HashChr32, 4 * 0, notBigendian); // ���Сѭ��
	UINT_2_UCHAR(V[1], sm3HashChr32, 4 * 1, notBigendian);
	UINT_2_UCHAR(V[2], sm3HashChr32, 4 * 2, notBigendian);
	UINT_2_UCHAR(V[3], sm3HashChr32, 4 * 3, notBigendian);
	UINT_2_UCHAR(V[4], sm3HashChr32, 4 * 4, notBigendian);
	UINT_2_UCHAR(V[5], sm3HashChr32, 4 * 5, notBigendian);
	UINT_2_UCHAR(V[6], sm3HashChr32, 4 * 6, notBigendian);
	UINT_2_UCHAR(V[7], sm3HashChr32, 4 * 7, notBigendian);

}

void SM3hmac(unsigned char msgText[], unsigned int keyInt16[], int notBigendian, unsigned char sm3hmacChr32[])
{
	// ��ΪĬ��ʹ���Լ����������key�������ʱ��涨���ɵ�key����64Byte���Ͳ���Ҫ���key��64Byte
	unsigned int tempInt16[32]; // ipad opadһ�����ˣ�tempIntǰ16��Ԫ�ش洢��ipad���������16��Ԫ�ش洢opad�����
	for (int i = 0; i < 16; i++) {
		tempInt16[i] = keyInt16[i] ^ 0x36363636;//ipad[i];
		tempInt16[i + 16] = keyInt16[i] ^ 0x5c5c5c5c;// opad[i];
	}
	unsigned char keyChr64[128]; // ǰ64�洢ipad�����(int)ת��char����64�洢opad
	for (int i = 0; i < 16; i++) {
		UINT_2_UCHAR(tempInt16[i], keyChr64, i << 2, notBigendian);
		UINT_2_UCHAR(tempInt16[i + 16], keyChr64, (i + 16) << 2, notBigendian);
	}
	unsigned char* jointChr = (unsigned char*)malloc(strlen(msgText) + 64);// ��һ��hashǰ��ƴ�ӳ���
	//unsigned char* jointChr = (unsigned char*)malloc((strlen(msgText) + 64) * sizeof(unsigned char));// ��һ��hashǰ��ƴ�ӳ���
	memcpy(jointChr, keyChr64, 64);
	memcpy(jointChr + 64, msgText, strlen(msgText));
	memset(jointChr + 64 + strlen(msgText), 0, 1);
	// һ��Ҫ��ĩβ����\0��־�ַ�����β�������������msgFill512ʱ���ȡmsgLength���д���

	SM3Hash(jointChr, notBigendian, sm3hmacChr32);
	//free(jointChr);// �ͷ��ڴ�
	//unsigned char* jointChr1 = (unsigned char*)malloc((64 + 32) * sizeof(unsigned char)); // �ڶ���hashǰ��ƴ�ӳ���
	unsigned char jointChr1[97];//Ҫ���1�������������0
	memcpy(jointChr1, keyChr64 + 64, 64);
	memcpy(jointChr1 + 64, sm3hmacChr32, 32);
	memset(jointChr1 + 96, 0, 1);
	SM3Hash(jointChr1, notBigendian, sm3hmacChr32);

}

void HmacPrint(unsigned char hmac32[])
{
	for (int i = 0; i < 32; i++) {
		printf("%02x", hmac32[i]);
		if (i != 0 && i % 4 == 0) {
			printf(" ");
		}
	}
	printf("\n");
}

void Key16Generate(unsigned int keyInt16[], int notbigendian)
{
	srand((unsigned int)time(0));
	unsigned char keyChr64[64];
	for (int i = 0; i < 64; i++) {
		while (1)
		{
			keyChr64[i] = rand() % (0xff - 0x00 + 0x01); //rand���� % ��b - a + 1��+ a 
			if (keyChr64[i] != 0x36 && keyChr64[i] != 0x5c) {
				break;
			}
		}
	}
	for (int i = 0; i < 16; i++) {
		UCHAR_2_UINT(keyChr64, keyInt16[i], 4 * i, notbigendian);
	}

}

void SM3hmacWithFile(char* filename, int fileChrAmount)
{
	FILE* fp;
	int bigendFlag = NOT_BIG_ENDIAN();
	unsigned char* msg = (unsigned char*)malloc((fileChrAmount + 1) * sizeof(unsigned char));
	unsigned int keyInt16[16];
	Key16Generate(keyInt16, bigendFlag);
	puts("ʹ�õ������Կ��");
	for (int i = 0; i < 16; i++) {
		printf("%08x ", keyInt16[i]);
	}
	printf("\n--------------------------------------------------------------------------\n");
	unsigned char sm3hmacValue[32];

	clock_t start, end;
	double excuteTime;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Cannot open  %s ! \n", filename);
		exit(0);
	}
	start = clock();
	fgets(msg, fileChrAmount + 1, fp); // +1ԭ������
	//char *fgets(char *str,int n,FILE *fp)
	//����fpָ�����ļ��ж�ȡn-1���ַ����������Ǵ�ŵ���strָ�����ַ�������ȥ��������һ���ַ���������'\0'
	end = clock();
	excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;
	printf("��Ϣ��ȡ��ɣ��� %d �ֽڣ���ʱ: %f seconds.\n", fileChrAmount, excuteTime);
	//printf("Your msg show as below:\n%s\n", msg);
	//printf("\n------------------------------------\nCharacter Amount: %d\n", strlen(msg));
	fclose(fp);

	start = clock();
	SM3hmac(msg, keyInt16, bigendFlag, sm3hmacValue);
	end = clock();
	excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;

	HmacPrint(sm3hmacValue);
	printf("���SM3hmac����ʱ��: % f seconds.\n", excuteTime);
	double speed = fileChrAmount / excuteTime;
	speed /= pow(2, 20);
	printf("SM3hmac�����ٶ�: %f MBps(%f Mbps)\n", speed, speed * 8);

}


void SM3Interface()
{
	puts("      RGRGRGRGR     RG    RG    RG  RGRGRGRGRG      ");
	puts("    RG              RG    RG    RG            RG    ");
	puts("    RG              RG    RG    RG            RG    ");
	puts("      RGRGRGRGR     RG    RG    RG    RGRGRGRG      ");
	puts("               RG   RG          RG            RG    ");
	puts("               RG   RG          RG            RG    ");
	puts("      RGRGRGRGR     RG          RG  RGRGRGRGRG      ");
	puts("                                                    ");
	puts("    RG      RG  RG  RG  RG     RGRG       RGRGRGR   ");
	puts("    RG      RG  RG  RG  RG  RG      RG  RG          ");
	puts("    RG RGRG RG  RG  RG  RG  RG  AG  RG  RG          ");
	puts("    RG      RG  RG      RG  RG      RG  RG       G  ");
	puts("    RG      RG  RG      RG  RG      RG    RGRGRGR   ");
	puts("");
	puts("  ================================================= ");
	puts(" |                                                 |");
	puts(" |               Welcome to SM3hamc !              |");
	puts(" |                 ��ӭʹ��SM3hmac ��              |");
	puts(" |                                                 |");
	puts(" |          ���������, ѡ����Ӧ����(��q�˳�)��    |");
	puts(" |                                                 |");
	puts(" |        1.SM3�ĵ�ʾ��   2.��������   3.�Զ���    |");
	puts(" |                                                 |");
	puts("  ================================================= ");

}

void SampleInterface()
{
	puts("  ================================================= ");
	puts(" |                                                 |");
	puts(" |     �������ṩ����8���������ڲ��Գ���ִ��Ч��   |");
	puts(" |                                                 |");
	puts(" |   ��������ţ�ѡ����Ӧ������ʾ���ļ�(��q�˳�)�� |");
	puts(" |                                                 |");
	puts(" |      (1) 1KB (2) 10KB (3) 50KB (4) 100KB        |");
	puts(" |      (5) 1MB (6) 10MB (7) 50MB (8) 100MB        |");
	puts(" |                                                 |");
	puts("  ================================================= ");
}

void CustomInterface()
{
	puts("  ================================================= ");
	puts(" |                                                 |");
	puts(" |   ��������ţ�ѡ����Ӧ���ܼ���HMAC(��q�˳�)��   |");
	puts(" |                                                 |");
	puts(" |      (1) ֱ��������Ϣ (2) ָ����Ϣ�ļ�          |");
	puts(" |                                                 |");
	puts("  ================================================= ");
}

// �ĵ�ʾ��һ
void Eg1_test() {
	int bigendFlag = NOT_BIG_ENDIAN();

	unsigned char* msg = "abc";
	//unsigned char* hashChr = SM3Hash_Old(chr, bigendFlag);
	unsigned char hashChr[32];

	SM3Hash(msg, bigendFlag, hashChr);

	puts("\n-------------------- �ĵ�ʾ��1 --------------------\n");
	//Fill_N_extend_test(chr);
	puts("ʾ��1 SM3�Ӵ�ֵ: ");
	for (int i = 0; i < 32; i++) {
		printf("%02x", hashChr[i]);
		if (i != 0 && i % 4 == 0) {
			printf(" ");
		}
	}
	printf("\n");
	printf("\n-------------------------------------------------\n");
	unsigned int keyInt16[16];
	Key16Generate(keyInt16, bigendFlag);
	puts("�����Կ��");
	for (int i = 0; i < 16; i++) {
		printf("%08x ", keyInt16[i]);
	}
	printf("\n-------------------------------------------------\n");
	unsigned char sm3hmacValue[32];
	clock_t start, end;
	double excuteTime;
	start = clock();
	SM3hmac(msg, keyInt16, bigendFlag, sm3hmacValue);
	end = clock();
	excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;
	puts("ʾ��1 HMAC��");
	HmacPrint(sm3hmacValue);
	printf("-------------------------------------------------\n");
	printf("���SM3hmac����ʱ��: % f seconds.\n", excuteTime);
	double speed = strlen(msg) / excuteTime;
	speed /= pow(2, 20);
	printf("SM3hmac�����ٶ�: %f MBps(%f Mbps)\n", speed, speed * 8);

}

// �ĵ�ʾ����
void Eg2_test() {
	int bigendFlag = NOT_BIG_ENDIAN();
	unsigned char* msg = "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd";

	//unsigned char* hashChr = SM3Hash_Old(chr, bigendFlag);
	// ����û���⣬���Ƿ��ظ�ָ��֮�������puts����ʱ�����ƻ�hashChrָ����ڴ�����
	unsigned char hashChr[32];

	SM3Hash(msg, bigendFlag, hashChr);

	puts("\n-------------------- �ĵ�ʾ��2 --------------------\n");
	//Fill_N_extend_test(chr);
	puts("ʾ��2 SM3�Ӵ�ֵ: ");
	for (int i = 0; i < 32; i++) {
		printf("%02x", hashChr[i]);
		if (i != 0 && i % 4 == 0) {
			printf(" ");
		}
	}
	printf("\n");
	printf("\n-------------------------------------------------\n");
	unsigned int keyInt16[16];
	Key16Generate(keyInt16, bigendFlag);
	puts("�����Կ��");
	for (int i = 0; i < 16; i++) {
		printf("%08x ", keyInt16[i]);
	}
	printf("\n-------------------------------------------------\n");
	unsigned char sm3hmacValue[32];
	clock_t start, end;
	double excuteTime;
	start = clock();
	SM3hmac(msg, keyInt16, bigendFlag, sm3hmacValue);
	end = clock();
	excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;
	puts("ʾ��2 HMAC��");
	HmacPrint(sm3hmacValue);
	printf("-------------------------------------------------\n");
	printf("���SM3hmac����ʱ��: % f seconds.\n", excuteTime);
	double speed = strlen(msg) / excuteTime;
	speed /= pow(2, 20);
	printf("SM3hmac�����ٶ�: %f MBps(%f Mbps)\n", speed, speed * 8);
}

void StdSM3Test()
{
	puts("------- �����ǹ���SM3��׼�ĵ�����������ʾ����SM3�Ӵ�ֵ��HMAC -------");
	Eg1_test();
	Eg2_test();

}

void SampleFileTest()
{
	while (1) {
		int outFlag = 0;
		char option;
		char* filename;
		int fileChrAmount;
		SampleInterface();
		puts("��ѡ����ţ�");
		option = getchar();
		getchar();//���ջس�
		switch (option)
		{
		case '1':
			system("clear");
			filename = ".//TestSample//msg1k.txt";
			fileChrAmount = 1389;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '2':
			system("clear");
			filename = ".//TestSample//msg10k.txt";
			fileChrAmount = 13748;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '3':
			system("clear");
			filename = ".//TestSample//msg50k.txt";
			fileChrAmount = 68956;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '4':
			system("clear");
			filename = ".//TestSample//msg100k.txt";
			fileChrAmount = 110433;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '5':
			system("clear");
			filename = ".//TestSample//msg1M.txt";
			fileChrAmount = 1411509;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '6':
			system("clear");
			filename = ".//TestSample//msg10M.txt";
			fileChrAmount = 11291574;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '7':
			system("clear");
			filename = ".//TestSample//msg50M.txt";
			fileChrAmount = 59351213;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '8':
			system("clear");
			filename = ".//TestSample//msg100M.txt";
			fileChrAmount = 108683394;
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case 'q':
			outFlag = 1;
			break;
		default:
			system("clear");
			puts("Unsupported Input!\nPlease rechoose!");
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		}
		if (outFlag)
			break;
	}
}

void CustomIput()
{
	int bigendFlag = NOT_BIG_ENDIAN();
	while (1) {
		int outFlag = 0;
		char option;
		clock_t start, end;
		double excuteTime;
		CustomInterface();
		puts("��ѡ����ţ�");
		option = getchar();
		getchar();//���ջس�
		unsigned int keyInt16[16];
		unsigned char sm3hmacValue[32];
		switch (option)
		{
		case '1':
			system("clear");
			puts("��������Ϣ�ַ���");
			unsigned char msg[1024];
			scanf("%s", msg);
			getchar();//���ջس�
			printf("\n-------------------------------------------------\n");
			Key16Generate(keyInt16, bigendFlag);
			puts("�����Կ��");
			for (int i = 0; i < 16; i++) {
				printf("%08x ", keyInt16[i]);
			}
			printf("\n-------------------------------------------------\n");
			start = clock();
			SM3hmac(msg, keyInt16, bigendFlag, sm3hmacValue);
			end = clock();
			excuteTime = ((double)end - (double)start) / CLOCKS_PER_SEC;
			puts("���������Ϣ�� HMAC��");
			HmacPrint(sm3hmacValue);
			printf("-------------------------------------------------\n");
			printf("���SM3hmac����ʱ��: % f seconds.\n", excuteTime);
			double speed = strlen(msg) / excuteTime;
			speed /= pow(2, 20);
			printf("SM3hmac�����ٶ�: %f MBps(%f Mbps)\n", speed, speed * 8);

			//system("pause");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '2':
			system("clear");
			puts("��ָ����Ϣ�ļ��ľ���·��(����ȷ�����ɽ��ļ����뱾���������ļ��У�Ȼ�������ļ������ɣ����ļ���׺.txt)��");
			char filename[1024];
			scanf("%s", filename);
			getchar();//���ջس�
			int fileChrAmount;
			puts("��������Ϣ�ļ��ֽ���(�ɲ����ļ����Ե�֪)��");
			scanf("%d", &fileChrAmount);
			getchar();//���ջس�
			SM3hmacWithFile(filename, fileChrAmount);
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case 'q':
			outFlag = 1;
			break;
		default:
			system("clear");
			puts("Unsupported Input!\nPlease rechoose!");
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		}
		if (outFlag)
			break;
	}
}

void SM3hmacEntry()
{
	while (1) {
		int outFlag = 0;
		char option;
		SM3Interface();
		puts("��ѡ����ţ�");
		option = getchar();
		getchar();//���ջس�
		switch (option)
		{
		case '1':
			system("clear");
			StdSM3Test();
			//system("pause");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '2':
			system("clear");
			SampleFileTest();
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case '3':
			system("clear");
			CustomIput();
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		case 'q':
			outFlag = 1;
			break;
		default:
			system("clear");
			puts("Unsupported Input!\nPlease rechoose!");
			//system("pause");
			//system("cls");
			puts("�����������...");
			if (getchar() != '\n') {
				getchar();
			}
			system("clear");
			break;
		}
		if (outFlag)
			break;
	}
}

int main()
{
	SM3hmacEntry();

	return 0;
}