#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<errno.h>
#include<math.h>

enum
{
    O32_LITTLE_ENDIAN = 0x03020100ul,
    O32_BIG_ENDIAN = 0x00010203ul,
    O32_PDP_ENDIAN = 0x01000302ul,      /* DEC PDP-11 (aka ENDIAN_LITTLE_WORD) */
    O32_HONEYWELL_ENDIAN = 0x02030001ul /* Honeywell 316 (aka ENDIAN_BIG_WORD) */
};


/*************************************************************************
  WAV Loading Func
*************************************************************************/
struct s_wav_header {
char ChunkID[4];
unsigned char ChunkSize[4];
char Format[4];
char SubChunk1ID[4];
unsigned char SubChunk1Size[4];
unsigned char AudioFormat[2];
unsigned char NumChannels[2];
unsigned char SampleRate[4];
unsigned char ByteRate[4];
unsigned char BlockAlign[2];
unsigned char BitsPerSample[2];
unsigned char SubChunk2ID[4];
unsigned char SubChunk2Size[4];
};

double (*_internal_getsample)(unsigned char *data, int *idx);

double __internal_getsample8(unsigned char *data, int *idx) {
        double v;
        v=data[*idx]-128;*idx=*idx+1;return v;
}
double __internal_getsample16(unsigned char *data, int *idx) {
	char *most;
        double v;
	most=(char*)data;
        v=most[*idx+1];v+=data[*idx]/256.0;*idx=*idx+2;return v;
}
double __internal_getsample32(unsigned char *data, int *idx) {
	float v,*peteher;
	peteher=(float*)(&data[*idx]);
        v=*peteher;
	*idx=*idx+4;
	return v*128.0;
}

double *load_wav(char *filename, int *n, double *acqui) {
	struct s_wav_header *wav_header;
	int controlsize,nbchannel,wFormat,frequency,bitspersample,nbsample;
        unsigned char *subchunk;
	unsigned char *data;
        int subchunksize;
	int filesize,idx;
	int i,num;
	FILE *f;
	double *wav;

	f=fopen(filename,"rb");
	if (!f) {
		fprintf(stderr,"file [%s] not found\n",filename);
		return NULL;
	}
	fseek(f,0,SEEK_END);
	filesize=ftell(f);
	fseek(f,0,SEEK_SET);

        if (filesize<sizeof(struct s_wav_header)) {
                fprintf(stderr,"WAV import - this file is too small to be a valid WAV!\n");
                return NULL;
        }

	data=(unsigned char *)malloc(filesize);
	fread(data,1,filesize,f);
	fclose(f);

        wav_header=(struct s_wav_header *)data;
        if (strncmp(wav_header->Format,"WAVE",4)) {
                fprintf(stderr,"WAV import - unsupported audio sample type (format must be 'WAVE')\n");
		free(data);
                return NULL;
        }
        controlsize=wav_header->SubChunk1Size[0]+wav_header->SubChunk1Size[1]*256+wav_header->SubChunk1Size[2]*65536+wav_header->SubChunk1Size[3]*256*65536;
        if (controlsize!=16) {
                fprintf(stderr,"WAV import - invalid wav chunk size (subchunk1 control) => %d!=16\n",controlsize);
        }
        if (strncmp(wav_header->SubChunk1ID,"fmt",3)) {
                fprintf(stderr,"WAV import - unsupported audio sample type (subchunk1id must be 'fmt')\n");
		free(data);
                return NULL;
        }

	if (controlsize==16) {
		subchunk=(unsigned char *)&wav_header->SubChunk2ID;
	} else {
		subchunk=(unsigned char *)&wav_header->SubChunk2ID-16+controlsize;
	}
	// tant qu on n est pas sur le chunk data, on avance dans le "TIFF"
	while (strncmp((char *)subchunk,"data",4)) {
		subchunksize=8+subchunk[4]+subchunk[5]*256+subchunk[6]*65536+subchunk[7]*256*65536;
		if (subchunksize>=filesize) {
			fprintf(stderr,"WAV import - data subchunk not found\n");
			free(data);
			return NULL;
		}
		subchunk+=subchunksize;
	}
	subchunksize=subchunk[4]+subchunk[5]*256+subchunk[6]*65536+subchunk[7]*256*65536;
	controlsize=subchunksize; // taille des samples

        nbchannel=wav_header->NumChannels[0]+wav_header->NumChannels[1]*256;
        if (nbchannel<1) {
                fprintf(stderr,"WAV import - invalid number of audio channel\n");
		free(data);
                return NULL;
        }

        wFormat=wav_header->AudioFormat[0]+wav_header->AudioFormat[1]*256;
        if (wFormat!=1 && wFormat!=3) {
                fprintf(stderr,"WAV import - invalid or unsupported wFormatTag (%04X) ",wFormat);
		switch (wFormat) {
			case -2:fprintf(stderr,"Extensible Format (user defined) \n");
			default:
			case 0:fprintf(stderr,"	Unknown Format\n");
			case 1:fprintf(stderr,"	PCM format (8 or 16 bit), Microsoft Corporation\n");
			case 2:fprintf(stderr,"	AD PCM Format, Microsoft Corporation\n");
			case 3:fprintf(stderr,"	IEEE PCM Float format (32 bit)\n");
			case 48:fprintf(stderr,"AC2, Dolby Laboratories\n");
			case 49:fprintf(stderr,"GSM 6.10, Microsoft Corporation\n");
			case 50:fprintf(stderr,"MSN Audio, Microsoft Corporation\n");
			case 80:fprintf(stderr,"MPEG format\n");
			case 85:fprintf(stderr,"ISO/MPEG Layer3 Format\n");
			case 146:fprintf(stderr,"AC3 Digital, Sonic Foundry\n");
			case 255:fprintf(stderr,"Raw AAC\n");
			case 352:fprintf(stderr,"Microsoft Corporation\n");
			case 353:fprintf(stderr,"Windows Media Audio. This format is valid for versions 2 through 9\n");
			case 354:fprintf(stderr,"Windows Media Audio 9 Professional\n");
			case 355:fprintf(stderr,"Windows Media Audio 9 Lossless\n");
			case 356:fprintf(stderr,"Windows Media SPDIF Digital Audio\n");
			case 5632:fprintf(stderr,"ADTS AAC Audio\n");
			case 5633:fprintf(stderr,"Raw AAC\n");
			case 5634:fprintf(stderr,"MPEG-4 audio transport stream with a synchronization layer (LOAS) and a multiplex layer (LATM)\n");
			case 5648:fprintf(stderr,"High-Efficiency Advanced Audio Coding (HE-AAC) stream\n");
		}
		free(data);
                return NULL;
        }

        frequency=wav_header->SampleRate[0]+wav_header->SampleRate[1]*256+wav_header->SampleRate[2]*65536+wav_header->SampleRate[3]*256*65536;
        bitspersample=wav_header->BitsPerSample[0]+wav_header->BitsPerSample[1]*256;

	switch (bitspersample) {
		case 8:_internal_getsample=__internal_getsample8;break;
		case 16:_internal_getsample=__internal_getsample16;break;
		case 32:_internal_getsample=__internal_getsample32;break;
		default:fprintf(stderr,"unsupported bits per sample size %d (valid are PCM-8, PCM-16 and FLOAT-32)\n",bitspersample); free(data);return NULL;
	}

        nbsample=controlsize/nbchannel/(bitspersample/8);
        if (controlsize+sizeof(struct s_wav_header)>filesize) {
                fprintf(stderr,"WAV import - cannot read %d byte%s of audio whereas the file is %d bytes big!\n",controlsize,controlsize>1?"s":"",filesize);
		free(data);
                return NULL;
        }

	*n=nbsample;

	fprintf(stderr,"nbSample=%d nbchannel=%d | bitpersample=%d | Format=%s | frequency=%d\n",nbsample,nbchannel,bitspersample,wFormat==1?"PCM":"IEEE Float",frequency);

	*acqui=frequency;
	wav=(double *)malloc(nbsample*sizeof(double));
	idx=subchunk+8-data;

       for (i=0;i<nbsample;i++) {
		/* downmixing */
		double accumulator=0.0;
		for (num=0;num<nbchannel;num++) {
			accumulator+=_internal_getsample(data,&idx);
		}
		wav[i]=accumulator/nbchannel;
	}

	return wav;
}

// numeric recipes cooking books routine
void passe_bande(double acqui,double basse,double haute,double *datain,double *dataout,int nb)
{
        double *datatmp=NULL;
        double A1,A2,B0,B1,B2;
        double a,b;
        int i;
        /***/
        double e,old_e,old_old_e;
        double s,old_s,old_old_s;

        if (nb<=0) return;
        if (basse<0.0001) basse=0.0001;
        if (haute<0.0001) haute=0.0001;
        if (basse>acqui) basse=acqui;
        if (haute>acqui) haute=acqui;

        datatmp=(double *)malloc(sizeof(double)*nb);
        if (!datatmp) return;

        a=M_PI*basse/acqui;
        b=M_PI*haute/acqui;
        a=sin(a)/cos(a);
        b=sin(b)/cos(b);
        B0= -b / ((1 + a) * (1 + b));
        B1= 0;
        B2= b / ((1 + a) * (1 + b));
        A1= ((1 + a) * (1 - b) + (1 - a) * (1 + b)) / ((1 + a) * (1 + b));
        A2= -(1 - a) * (1 - b) / ((1 + a) * (1 + b));

        /* init */
        old_e=old_old_e=datain[0];
        old_s=old_old_s=0;

	/* a filter phases out signal */
        for (i=0;i<nb;i++) {
                e=datain[i];
                s=e*B0+old_e*B1+old_old_e*B2+old_s*A1+old_old_s*A2;
                datatmp[i]=s;
                old_old_s=old_s;
                old_s=s;
                old_old_e=old_e;
                old_e=e;
        }
        old_e=old_old_e=e;
        old_s=old_old_s=0;

	/* so we filter again reverse */
        while (i>0) {
                i--;
                e=datatmp[i];
                s=e*B0+old_e*B1+old_old_e*B2+old_s*A1+old_old_s*A2;
                dataout[i]=s;
                old_old_s=old_s;
                old_s=s;
                old_old_e=old_e;
                old_e=e;
        }
        free(datatmp);
}

int getToken(int v) {
	if (v>=8) return 7; else
	if (v>=4) return 6; else
	if (v>=2) return 5; else
	if (v>=1) return 4; else
	if (v==0) return 3; else
	if (v<=-4) return 0; else
	if (v<=-2) return 1; else
		return 2;
}

void do_sample(double *data,int nbsample, double preamp, double cutlow, double cuthigh, double acqui) {
	double *newdata;
	unsigned char *dataout;
	unsigned char volume[256];
	double smp1,smp2;
	int i,j,v,o,o1,o2;
	int pv,v1,v2;
	int op1,op2;
	int delta[8]={-4,-2,-1,0,1,2,4,8};

	int cb=0,part=-1,tok,ptok=-1;

	if (acqui<6000 || acqui>50000) {
		fprintf(stderr,"wrong acquisition frequency. Default value is 15.6KHz\n");
		acqui=15600.0;
	}
	if (cuthigh<cutlow || cutlow<0.0 || cuthigh>acqui) {
		fprintf(stderr,"wrong band-pass filter frequencies. Default value are 10Hz-2500Hz\n");
		cuthigh=2500.0;
		cutlow=10;
	}

	if (preamp!=1.0) for (i=0;i<nbsample;i++) data[i]*=preamp; // pre-amp THEN filter

	// filtrage
	fprintf(stderr,"band-pass %.1lf|%.1lf preamp=%.1lf\n",cutlow,cuthigh,preamp);
	newdata=(double *)malloc(sizeof(double)*nbsample);
	passe_bande(acqui,cutlow,cuthigh,data,newdata,nbsample);
	memcpy(data,newdata,nbsample*sizeof(double));
	free(newdata);
	newdata=NULL;

	// log volume gives REALLY better results
        for (i=j=0;i<1;i++) volume[j++]=0;
        for (i=0;i<1;i++)  volume[j++]=1;
        for (i=0;i<1;i++)  volume[j++]=2;
        for (i=0;i<2;i++)  volume[j++]=3;
        for (i=0;i<3;i++)  volume[j++]=4;
        for (i=0;i<4;i++)  volume[j++]=5;
        for (i=0;i<5;i++)  volume[j++]=6;
        for (i=0;i<6;i++)  volume[j++]=7;
        for (i=0;i<9;i++)  volume[j++]=8;
        for (i=0;i<18;i++) volume[j++]=9;
        for (i=0;i<20;i++) volume[j++]=10;
        for (i=0;i<24;i++) volume[j++]=11;
        for (i=0;i<30;i++) volume[j++]=12;
        for (i=0;i<34;i++) volume[j++]=13;
        for (i=0;i<48;i++) volume[j++]=14;
        for (i=0;i<50;i++) volume[j++]=15;

	// traiter tous les samples
	dataout=malloc(nbsample);
	pv=0x0C;
	i=o=0;
	for (i=0;i<nbsample;i++) {
		smp1=data[i]+128;
		if (smp1<0) smp1=0; else if (smp1>255) smp1=255;
		v1=volume[(unsigned char)smp1];

		// différence delta
		tok=getToken(v1-pv);
		// on modifie la valeur courante (pour gérer les compensations)
		pv+=delta[tok];
		if (pv<0 || pv>15) {
			printf("invalid volume at offset %d!\n",i);
			exit(-2);
		}
		// on stocke le token delta
		dataout[o++]=tok;
	}
printf("%d tokens (should be %d as nbsample)\n",o,nbsample);
	// compresser les tokens si repetitions
	o1=0;o2=0;
	while (o1+1<o) {
		if (dataout[o1]==dataout[o1+1]) {
			dataout[o2++]=dataout[o1]|8;
			o1+=2;
		} else {
			dataout[o2++]=dataout[o1];
			o1++;
		}
	}
printf("%d tokens apres pack des repetitions\n",o2);

	o=o2;
	o1=o2=0;
	while (o2<o) {
		dataout[o1++]=(dataout[o2]<<4)|dataout[o2+1];
		o2+=2;
	}
printf("%d tokens apres repack global\n",o1);

	// ecriture
{
	FILE *f;
	f=fopen("output.bin","wb");
	if (!f) {
		printf("impossible d'ouvrir output.bin en ecriture\n");
		exit(-1);
	}
	fwrite(dataout,1,o1,f);
	fclose(f);
	printf("fichier output.bin ecrit\n");
}
}


void usage() {
	printf("<wafile> -preamp -high -low\n");
	exit(1);
}

void main(int argc, char **argv) {
	double *data;
	double frequency;
	double preamp=1.0;
	double cuthigh=4000;
	double cutlow=10;
	int ifilename=-1;
	int nbsamples;
	int i;

	for (i=1;i<argc;i++) {
		if (strcmp(argv[i],"-preamp")==0 && i+1<argc) {
			preamp=atof(argv[++i]);
		} else if (strcmp(argv[i],"-high")==0 && i+1<argc) {
			cuthigh=atof(argv[++i]);
		} else if (strcmp(argv[i],"-low")==0 && i+1<argc) {
			cutlow=atof(argv[++i]);
		} else if (ifilename==-1) {
			ifilename=i;
		} else usage();
	}

	if (ifilename==-1) usage();

	if ((data=load_wav(argv[ifilename],&nbsamples,&frequency))!=NULL) {
		do_sample(data,nbsamples,preamp,cutlow,cuthigh,frequency);
	}
}

