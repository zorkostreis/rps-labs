#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 1024 // ������ ������ 
#define MAXMES 4 // ������������ ���������� ������������ ���������

char* errors[32] = // ������� �����������
{
	"Connection is successful       \n",
	"Unable to create socket        \n",
	"Unable to connect to POP server\n",
	"Authentication failed          \n"
};

// ������� ������ ������ � ������� i �� ������ sock
void readMail(int sock, int i)
{
	int n;
	char buf[BUFSIZE];
	int f;

	sprintf(buf, "RETR %d\n", i); // ������ � ������� ���� ������
	write(sock, buf, strlen(buf));

	printf("\n\nMessage �%d\n----------------||----------------\n", i); fflush(stdout);

	do {
		n = recv(sock, buf, BUFSIZE, 0); // ������ ����� ��������� � buf
		f = memcmp("\r\n.\r\n", buf + n - 5, 5); // �������� ����� ���������
		write(1, buf, n); // ����� ����������� buf 
	} while (f); // ���� ����������� �� CRLF.CRLF

	write(1, "\n----------------||----------------\n\n", 38);
}

int initMail(int* sock, char* confFile)
{
	struct sockaddr_in csin, ssin;
	struct hostent* hp;
	int n;
	char buf[BUFSIZE];
	FILE* fd;
	char servn[32], usern[32], passn[32]; // ��� �������, ����� � ������

	fd = fopen(confFile, "r"); // ���� � ������ �������, ������� � �������
	fscanf(fd, "%s\n%s\n%s", servn, usern, passn);
	fclose(fd);

	// ������������� TCP ��������-������ 
	if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return 1;

	// ���������� ��������� ssin 
	memset((char*)&ssin, '\0', sizeof(ssin));
	ssin.sin_family = AF_INET;
	hp = gethostbyname(servn); // ���������� ��������� ����� 
	memcpy((char*)&ssin.sin_addr, hp->h_addr, hp->h_length);
	ssin.sin_port = htons(110); // 110�� ����� ����� ������������� � ����������� ������

	// ���������� ��������� csin 
	memset((char*)&csin, '\0', sizeof(csin));
	csin.sin_family = AF_INET;
	csin.sin_port = htons(12524);  // ������������ ����

	// �������� csin � ������ 
	bind(*sock, (struct sockaddr*)&csin, sizeof(csin));

	// ����������� � ������� �� ������ 
	if (connect(*sock, (struct sockaddr*)&ssin, sizeof(ssin)) == -1)
		return 2;

	buf[0] = 0;
	n = read(*sock, buf, BUFSIZE); // �������� ������ �� ������� 

	// �������� ������ 
	sprintf(buf, "USER %s\n", usern);
	write(*sock, buf, strlen(buf));
	n = read(*sock, buf, BUFSIZE);
	if (buf[0] == '-') return 3; // �������

	// �������� ������ 
	sprintf(buf, "PASS %s\n", passn);
	write(*sock, buf, strlen(buf));
	n = read(*sock, buf, BUFSIZE);
	if (buf[0] == '-') return 3;

	return 0;
}

// ��������� ���������� ��������� � �����
int getCntM(int sock)
{
	int n;
	char buf[BUFSIZE];

	write(sock, "STAT\n", 5); // ������� ��� ��������� ���-�� ���������
	n = read(sock, buf, BUFSIZE);
	sscanf(buf + 3, "%d", &n);
	return n;
}

int main(int argc, char** argv) {
	char buf[BUFSIZE];
	int n;
	int sock; // ���������� ������ 

	if (argc < 2) { // ���� �� �������� ��� ����� � ���������� ���������
		write(1, "Missing argument 'config_file_name.cfg'\n", 40);
		return -1;
	}

	n = initMail(&sock, argv[1]);
	write(1, errors[n], 32); // ����� ������� �����������
	if (!n) { // ���� ���� �������� ����������� 
		n = getCntM(sock);
		for (int i = 0; i < n && i < MAXMES; ++i) // ������ n ����� �� ������ MAXMES
			readMail(sock, i + 1);
	}
	write(sock, "QUIT\n", 5); // ��������� ������� � ������ 
	shutdown(sock, 2);  // ������������ ���������� � �������� ������ 
	close(sock);
}