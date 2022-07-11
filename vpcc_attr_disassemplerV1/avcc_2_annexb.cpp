#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define NALU_TYPE_SLICE 1
#define NALU_TYPE_DPA 2
#define NALU_TYPE_DPB 3
#define NALU_TYPE_DPC 4
#define NALU_TYPE_IDR 5
#define NALU_TYPE_SEI 6
#define NALU_TYPE_SPS 7
#define NALU_TYPE_PPS 8
#define NALU_TYPE_AUD 9
#define NALU_TYPE_EOSEQ 10
#define NALU_TYPE_EOSTREAM 11
#define NALU_TYPE_FILL 12

/*
H264֡��NALUͷ��NALU������ɡ�
NALUͷ��һ���ֽ����,�����﷨����:

  +---------------+
  |0|1|2|3|4|5|6|7|
  +-+-+-+-+-+-+-+-+
  |F|NRI|  Type   |
  +---------------+

F: 1������.
  forbidden_zero_bit. �� H.264 �淶�й涨����һλ����Ϊ 0.

NRI: 2������.
  nal_ref_idc. ȡ00~11,�ƺ�ָʾ���NALU����Ҫ��,
  ��00��NALU���������Զ���������Ӱ��ͼ��Ļط�,0��3,
  ȡֵԽ�󣬱�ʾ��ǰNALԽ��Ҫ����Ҫ�����ܵ�����.
  �����ǰNAL�����ڲο�֡��Ƭ���������в�������
  ����ͼ���������Щ��Ҫ�ĵ�λʱ�����䷨Ԫ�ر������0��

Type: 5������.
  nal_unit_type. ���NALU��Ԫ������,1��12��H.264ʹ�ã�
  24��31��H.264�����Ӧ��ʹ��.
 */
typedef struct {
	uint8_t nal_type : 5;
	uint8_t nal_ref_idc : 2;
	uint8_t forbidden_zero_bit : 1;
} H264Hdr;

int parase_nalu_hdr(uint8_t* nal_hdr, int nal_len) {
	const char* type_str = NULL;
	H264Hdr* hdr = (H264Hdr*)nal_hdr;
	printf("### DUMP nalu hdr(hdr:0x%02x, len=%d):\n", *nal_hdr, nal_len);
	printf("forbidden_zero_bit:%d\n", hdr->forbidden_zero_bit);
	printf("nal_ref_idc:%d\n", hdr->nal_ref_idc);
	switch (hdr->nal_type) {
	case NALU_TYPE_SLICE:
		type_str = "P/B Slice";
		break;
	case NALU_TYPE_AUD:
		type_str = "AUD Slice";
		break;
	case NALU_TYPE_DPA:
		type_str = "DPA Slice";
		break;
	case NALU_TYPE_DPB:
		type_str = "DPB Slice";
		break;
	case NALU_TYPE_DPC:
		type_str = "DPC Slice";
		break;
	case NALU_TYPE_EOSEQ:
		type_str = "EOS seq Slice";
		break;
	case NALU_TYPE_EOSTREAM:
		type_str = "EOS stream Slice";
		break;
	case NALU_TYPE_FILL:
		type_str = "FILL Slice";
		break;
	case NALU_TYPE_IDR:
		type_str = "IDR Slice";
		break;
	case NALU_TYPE_PPS:
		type_str = "PPS Slice";
		break;
	case NALU_TYPE_SPS:
		type_str = "SPS Slice";
		break;
	case NALU_TYPE_SEI:
		type_str = "SEI Slice";
		break;
	default:
		type_str = "Unknow Slice";
		printf("Warn: nal type:%d\n", hdr->nal_type);
		break;
	}
	printf("nal_type:%s\n", type_str);
	return 0;
}

/*
 bits:
	8   version ( always 0x01 )
	8   avc profile ( sps[0][1] )
	8   avc compatibility ( sps[0][2] )
	8   avc level ( sps[0][3] )
	6   reserved ( all bits on )
	2   NALULengthSizeMinusOne  // ���ֵ�ǣ�ǰ׺����-1����ֵ�����3����ǰ׺����4����Ϊ4-1=3
	3   reserved ( all bits on )
	5   number of SPS NALUs (usually 1)
repeated once per SPS:
	16     SPS size
	variable   SPS NALU data
	8   number of PPS NALUs (usually 1)
repeated once per PPS
	16    PPS size
	variable PPS NALU data
 */

typedef struct {
	uint8_t version;
	uint8_t avc_profile;
	uint8_t avc_compatibility;
	uint8_t avc_level;
	uint8_t reserved0 : 6;
	uint8_t nalu_len : 2;
} AvccExtraHdr;

#define SPS_CNT_MASK 0x1F

int parase_avcc_extra(char* buff, int size) {
	uint8_t sps_cnt;
	uint16_t sps_size;
	uint8_t pps_cnt;
	uint16_t pps_size;
	uint8_t* offset = NULL;
	uint8_t extra_to_anexb[1024] = { 0x00, 0x00, 0x00, 0x01, 0x00 };

	if (!buff || (size < 7))
		return -1;

	AvccExtraHdr* hdr = (AvccExtraHdr*)buff;
	printf("version:0x%02x\n", hdr->version);
	printf("avc_profile:0x%02x\n", hdr->avc_profile);
	printf("avc_compatibility:0x%02x\n", hdr->avc_compatibility);
	printf("avc_level:0x%02x\n", hdr->avc_level);
	printf("reserved0:0x%02x\n", hdr->reserved0);
	printf("nalu_len:0x%02x\n", hdr->nalu_len);

	FILE* file_out = fopen("annexb.h264", "wb");
	if (!file_out) {
		printf("ERROR: open annexb.h264 failed!\n");
		return -1;
	}
	fseek(file_out, 0, SEEK_SET);

	offset = (uint8_t*)buff + sizeof(AvccExtraHdr);
	sps_cnt = *offset & SPS_CNT_MASK;
	offset++;
	printf("sps_cnt:%d\n", sps_cnt);
	for (int i = 0; i < sps_cnt; i++) {
		// �����ֽ���תΪ�����ֽ���
		sps_size = ntohs(*((uint16_t*)offset));
		offset += 2;
		printf("+++ FLC-DBG: write anexb hdr to annexb.h264...\n");
		fwrite(extra_to_anexb, 1, 4, file_out);
		printf("+++ FLC-DBG: write sps:0x%02x to annexb.h264...\n", *offset);
		fwrite(offset, 1, sps_size, file_out);
		printf("[%d] sps_size:%d, sps_data:", i, sps_size);
		for (int j = 0; j < sps_size; j++)
			printf("0x%02x ", *(offset++));
		printf("\n");
	}
	pps_cnt = *(offset++);
	printf("pps_cnt:%d\n", pps_cnt);
	for (int i = 0; i < sps_cnt; i++) {
		// �����ֽ���תΪ�����ֽ���
		pps_size = ntohs(*((uint16_t*)offset));
		offset += 2;
		printf("+++ FLC-DBG: write anexb hdr to annexb.h264...\n");
		fwrite(extra_to_anexb, 1, 4, file_out);
		printf("+++ FLC-DBG: write sps:0x%02x to annexb.h264...\n", *offset);
		fwrite(offset, 1, pps_size, file_out);
		printf("[%d] pps_size:%d, pps_data:", i, pps_size);
		for (int j = 0; j < pps_size; j++)
			printf("0x%02x ", *(offset++));
		printf("\n");
	}

	fclose(file_out);

	return 0;
}

#define PARASE_MODE_EXTRA 1
#define PARASE_MODE_STREAM 2

int main(int argc, char* argv[]) {
	int mode = 0;
	if (argc < 3) {
		printf("ERROR: Invalid args\n");
		printf("Usage: \n\t1.%s -extra avcc_extra.bin\n", argv[0]);
		printf("\t2.%s -stream avcc_stream.bin\n", argv[0]);
		return -1;
	}

	if (!strcmp(argv[1], "-extra"))
		mode = PARASE_MODE_EXTRA;
	else if (!strcmp(argv[1], "-stream"))
		mode = PARASE_MODE_STREAM;
	else {
		printf("ERROR: Mode invalid\n");
		return -1;
	}

	printf("# InFile:%s\n", argv[2]);
	FILE* file = fopen(argv[2], "r");
	if (!file) {
		printf("ERROR: open %s failed!\n", argv[2]);
		return -1;
	}

	fseek(file, 0, SEEK_END);
	int flen = ftell(file);
	if (flen <= 0) {
		printf("ERROR: file length invalid! flen:%d\n", flen);
		fclose(file);
		return -1;
	}

	uint8_t* buffer = (uint8_t*)malloc(flen + 1);
	if (!buffer) {
		printf("ERROR: malloc %d Bytes failed!\n", flen);
		fclose(file);
		return -1;
	}

	fseek(file, 0, SEEK_SET);
	int ret = fread(buffer, 1, flen, file);
	if (ret <= 0) {
		printf("ERROR: read file %d Bytes error!\n", flen);
		fclose(file);
		free(buffer);
		return -1;
	}
	fclose(file);

	if (mode == PARASE_MODE_EXTRA) {
		printf("#### PARASE AVCC EXTRA DATA ####\n");
		ret = parase_avcc_extra(buffer, ret);
		if (ret < 0)
			printf("ERROR: parase avcc data failed!\n");
	}
	else {
		printf("#### PARASE AVCC STREAM ####\n");
		// ����avcc h264����������Ĭ�ϣ�4�ֽڣ��ɸ���extra data��nalu_len�����㡣
		int offset = 0;
		int nal_len = 0;
		while (offset < flen) {
			nal_len = ntohl(*((uint32_t*)(buffer + offset)));
			// ��avcc��4�ֽڳ���ת��Ϊannex b�ġ�00 00 00 01���ָ�����
			*(buffer + offset) = 0;
			*(buffer + offset + 1) = 0;
			*(buffer + offset + 2) = 0;
			*(buffer + offset + 3) = 1;
			offset += 4;
			parase_nalu_hdr((uint8_t*)(buffer + offset), nal_len);
			offset += nal_len;
		}
		// ׷����ʽ���ļ�������ת�����Stream��
		FILE* file_out = fopen("annexb.h264", "a+");
		if (!file_out) {
			printf("ERROR: open annexb.h264 failed!\n");
			fclose(file);
			free(buffer);
			return -1;
		}

		fwrite(buffer, 1, flen, file_out);
		fclose(file_out);
	}

	free(buffer);
	return ret;
}