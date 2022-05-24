#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 1024 // размер буфера 
#define MAXMES 4 // Максимальное количество отображаемых сообщений

char* errors[32] = // Статусы подключения
{
	"Connection is successful       \n",
	"Unable to create socket        \n",
	"Unable to connect to POP server\n",
	"Authentication failed          \n"
};

// Функция чтения письма с номером i из сокета sock
void readMail(int sock, int i)
{
	int n;
	char buf[BUFSIZE];
	int f;

	sprintf(buf, "RETR %d\n", i); // Запрос к серверу дать письмо
	write(sock, buf, strlen(buf));

	printf("\n\nMessage №%d\n----------------||----------------\n", i); fflush(stdout);

	do {
		n = recv(sock, buf, BUFSIZE, 0); // Запись части сообщения в buf
		f = memcmp("\r\n.\r\n", buf + n - 5, 5); // Ожидание конца сообщения
		write(1, buf, n); // Вывод содержимого buf 
	} while (f); // Приём завершается по CRLF.CRLF

	write(1, "\n----------------||----------------\n\n", 38);
}

int initMail(int* sock, char* confFile)
{
	struct sockaddr_in csin, ssin;
	struct hostent* hp;
	int n;
	char buf[BUFSIZE];
	FILE* fd;
	char servn[32], usern[32], passn[32]; // Имя сервера, логин и пароль

	fd = fopen(confFile, "r"); // Файл с именем сервера, логином и паролем
	fscanf(fd, "%s\n%s\n%s", servn, usern, passn);
	fclose(fd);

	// Инициализация TCP интернет-сокета 
	if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return 1;

	// Заполнение структуры ssin 
	memset((char*)&ssin, '\0', sizeof(ssin));
	ssin.sin_family = AF_INET;
	hp = gethostbyname(servn); // Разрешение доменного имени 
	memcpy((char*)&ssin.sin_addr, hp->h_addr, hp->h_length);
	ssin.sin_port = htons(110); // 110ый номер порта преобразуется в необходимый формат

	// Заполнение структуры csin 
	memset((char*)&csin, '\0', sizeof(csin));
	csin.sin_family = AF_INET;
	csin.sin_port = htons(12524);  // Произвольный порт

	// Привязка csin к сокету 
	bind(*sock, (struct sockaddr*)&csin, sizeof(csin));

	// Подключение к серверу по сокету 
	if (connect(*sock, (struct sockaddr*)&ssin, sizeof(ssin)) == -1)
		return 2;

	buf[0] = 0;
	n = read(*sock, buf, BUFSIZE); // Ожидание ответа от сервера 

	// Отправка логина 
	sprintf(buf, "USER %s\n", usern);
	write(*sock, buf, strlen(buf));
	n = read(*sock, buf, BUFSIZE);
	if (buf[0] == '-') return 3; // Неудача

	// Отправка пароля 
	sprintf(buf, "PASS %s\n", passn);
	write(*sock, buf, strlen(buf));
	n = read(*sock, buf, BUFSIZE);
	if (buf[0] == '-') return 3;

	return 0;
}

// Получение количества сообщений в ящике
int getCntM(int sock)
{
	int n;
	char buf[BUFSIZE];

	write(sock, "STAT\n", 5); // Команда для получения кол-ва сообщений
	n = read(sock, buf, BUFSIZE);
	sscanf(buf + 3, "%d", &n);
	return n;
}

int main(int argc, char** argv) {
	char buf[BUFSIZE];
	int n;
	int sock; // Дескриптор сокета 

	if (argc < 2) { // Если не передано имя файла в аргументах программы
		write(1, "Missing argument 'config_file_name.cfg'\n", 40);
		return -1;
	}

	n = initMail(&sock, argv[1]);
	write(1, errors[n], 32); // Вывод статуса подключения
	if (!n) { // Если было успешное подключение 
		n = getCntM(sock);
		for (int i = 0; i < n && i < MAXMES; ++i) // Чтение n писем не больше MAXMES
			readMail(sock, i + 1);
	}
	write(sock, "QUIT\n", 5); // Сообщение серверу о выходе 
	shutdown(sock, 2);  // Обязательное отключение и закрытие сокета 
	close(sock);
}