#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define END_OF_COMMAND_LINE		"\r\n"
#define	MAX_SIZE_OF_VALUE_BUFFER	1024 * 1024 * 1
#define	FILE_NEW_DATA			0
#define	FILE_STAT			1
#define FILE_CONTEXT			2
#define	ZECACHE_MAX_FILE_SIZE	1024*1024*2
#define SORT_KEYNAME			"sort_for_keyname"
struct field_value {
	char		field[512];
	char		value[MAX_SIZE_OF_VALUE_BUFFER];
	long long	size;
};

struct redis_window {
	int			sockfd;
	int			port;
	char			ipaddr[32];
	char			keyname[512];
	struct field_value	data[3];

};


static int merge_data(char *dest, char *src)
{
	if (src)
		strncat(dest, src, strlen(src));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	return strlen(dest);
}

static int merge_int_value(char *dest, int src)
{
	char		buffer[1024] = {0, };
	char		str_len[32] = {0};

	snprintf(buffer, sizeof(buffer), "%d", src);
	snprintf(str_len, sizeof(str_len), "$%ld", strlen(buffer));
	strncat(dest, str_len, strlen(str_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, buffer, strlen(buffer));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_hset_method_opt(char *dest)
{
	char *get_method_head = "*4";
	char *get_method_len = "$4";
	char *get_method_opt = "HSET";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_zadd_method_opt(char *dest)
{
	char *get_method_head = "*4";
	char *get_method_len = "$4";
	char *get_method_opt = "ZADD";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_hsetnx_method_opt(char *dest)
{
	char *get_method_head = "*4";
	char *get_method_len = "$6";
	char *get_method_opt = "HSETNX";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_hmset_method_opt(char *dest, int num)
{
	char get_method_head[32] = {0, };
	char *get_method_len = "$5";
	char *get_method_opt = "HMSET";

	snprintf(get_method_head, sizeof(get_method_head), "*%d", num * 2 + 2);
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_hexists_method_opt(char *dest)
{
	char *get_method_head = "*3";
	char *get_method_len = "$7";
	char *get_method_opt = "HEXISTS";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_hget_method_opt(char *dest)
{
	char *get_method_head = "*3";
	char *get_method_len = "$4";
	char *get_method_opt = "HGET";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_hmget_method_opt(char *dest, int num)
{
	char get_method_head[1024] = {0, };
	char *get_method_len = "$5";
	char *get_method_opt = "HMGET";

	snprintf(get_method_head, sizeof(get_method_head), "*%d", num + 2);
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_hdel_method_opt(char *dest, int num)
{
	char get_method_head[1024] = {0, };
	char *get_method_len = "$4";
	char *get_method_opt = "HDEL";

	snprintf(get_method_head, sizeof(get_method_head), "*%d", num + 2);
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_hgetall_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$7";
	char *get_method_opt = "HGETALL";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_hvals_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$5";
	char *get_method_opt = "HVALS";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_hkeys_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$5";
	char *get_method_opt = "HKEYS";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_set_method_opt(char *dest)
{
	char *get_method_head = "*3";
	char *get_method_len = "$3";
	char *get_method_opt = "SET";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_append_method_opt(char *dest)
{
	char *get_method_head = "*3";
	char *get_method_len = "$6";
	char *get_method_opt = "append";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_strlen_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$6";
	char *get_method_opt = "strlen";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_hlen_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$4";
	char *get_method_opt = "hlen";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_hincrby_method_opt(char *dest)
{
	char *get_method_head = "*4";
	char *get_method_len = "$7";
	char *get_method_opt = "hincrby";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_incr_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$4";
	char *get_method_opt = "incr";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}



static int merge_decr_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$4";
	char *get_method_opt = "decr";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_del_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$3";
	char *get_method_opt = "del";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_get_method_opt(char *dest)
{
	char *get_method_head = "*2";
	char *get_method_len = "$3";
	char *get_method_opt = "GET";
	strncat(dest, get_method_head, strlen(get_method_head));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_len, strlen(get_method_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, get_method_opt, strlen(get_method_opt));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_key_name(char *dest, const char *keyname)
{
	char		keyname_len[32] = {0, };

	snprintf(keyname_len, sizeof(keyname_len), "$%ld", strlen(keyname));
	strncat(dest, keyname_len, strlen(keyname_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, keyname, strlen(keyname));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}

static int merge_field_name(char *dest, const char *field)
{
	char		field_len[32] = {0, };

	snprintf(field_len, sizeof(field_len), "$%ld", strlen(field));
	strncat(dest, field_len, strlen(field_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);
	strncat(dest, field, strlen(field));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


static int merge_value_len(char *dest, int len)
{
	char            valuename_len[32] = {0, };

	snprintf(valuename_len, sizeof(valuename_len), "$%d", len);
	strncat(dest, valuename_len, strlen(valuename_len));
	strncat(dest, END_OF_COMMAND_LINE, 2);

	return strlen(dest);
}


int get_value_by_key(char *dest, struct redis_window *window, int *skip)
{
	int		i = 0;
	int		ret = 0;
	int		count = 0;
	char 		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0,};
	char		value_size_str[32] = {0, };
	char		*p;
	int		value_size = 0;

	merge_get_method_opt(buffer);
	ret = merge_key_name(buffer, window->keyname);

	write(window->sockfd, buffer, ret);


	//	memset(buffer, 0, sizeof(buffer));

	ret = 0;
	p = dest;
	//	p = buffer;
	memset(p, 0, MAX_SIZE_OF_VALUE_BUFFER);


	while ((count = read(window->sockfd, p, MAX_SIZE_OF_VALUE_BUFFER)) > 0) {
		//		memcpy(p, buffer, count);
		if (ret == 0) {


			ret += count;
			/*the 0x24 ascii is $ */
			if (*p == 0x24) {
				p++;
				for (i = 0; (*p != '\r') && (*p != '\n'); i++, p++) {

					value_size_str[i] = *p;
				}
			}

			value_size = atoll(value_size_str);
			p += 2;
			/*because it move pointer three char space use p
			 * so is add 3, and loop i times*/
			*skip = 3 + i;
			count = count - 3 - i;
		} else 
		{
			ret += count;
		}


		/*skip of the '$' '\r' '\n' '\r' '\n' 5 character */
		p += count;
		if (ret == value_size + strlen(value_size_str) + 5) break;
	}



	return value_size;
}

#define redis_op_get		get_value_by_key

int	redis_op_append(struct redis_window *window, char *value, int value_len)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	int			ret = 0;

	p = buffer;
	merge_append_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = merge_value_len(p, value_len);
	write(window->sockfd, p, ret);

	while ((ret = write(window->sockfd, value, value_len) > 0)) {
		if (ret < value_len) {
			break;
		}
		value += ret;
	}
	write(window->sockfd, END_OF_COMMAND_LINE, 2);

	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));

	return 0;
}

int	redis_op_hset(struct redis_window *window, char *value, char *field, int value_len)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	char			*tmp_p = NULL;
	int			ret = -1;

	p = buffer;
	merge_hset_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = merge_field_name(p, window->data[0].field);
	ret = merge_value_len(p, value_len);
	write(window->sockfd, p, ret);

	while ((ret = write(window->sockfd, value, value_len) > 0)) {
		if (ret < value_len) {
			break;
		}
		value += ret;
	}
	write(window->sockfd, END_OF_COMMAND_LINE, 2);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		tmp_p = p;
		while (*tmp_p != '\r' && *tmp_p != '\n') {
			tmp_p++;
		}
		tmp_p = '\0';
		ret =atoi(p);
	} else {
		ret = -1;
	}

	return ret;
}


int	redis_op_hsetnx(struct redis_window *window, char *value, char *field, int value_len)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	char			*tmp_p = NULL;
	int			ret = -1;

	p = buffer;
	merge_hsetnx_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = merge_field_name(p, window->data[FILE_CONTEXT].field);
	ret = merge_value_len(p, value_len);
	write(window->sockfd, p, ret);

	while ((ret = write(window->sockfd, value, value_len) > 0)) {
		if (ret < value_len) {
			break;
		}
		value += ret;
	}
	write(window->sockfd, END_OF_COMMAND_LINE, 2);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		tmp_p = p;
		while (*tmp_p != '\r' && *tmp_p != '\n') {
			tmp_p++;
		}
		tmp_p = '\0';
		ret =atoi(p);
	} else {
		ret = -1;
	}

	return ret;
}


int	redis_op_hmset(struct redis_window *window, int num)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	char			*tmp_p = NULL;
	int			ret = -1;
	int			i = 0;

	p = buffer;
	merge_hmset_method_opt(p, num);
	ret = merge_key_name(p, window->keyname);

	for (i = 0; i < num; i++) {
		ret = merge_field_name(p, window->data[i].field);
		ret = merge_value_len(p, window->data[i].size);
		write(window->sockfd, p, ret);


		memset(p, 0, strlen(p));
		while ((ret = write(window->sockfd, 
						window->data[i].value, 
						window->data[i].size) > 0)) {
			if (ret < window->data[i].size) {
				break;
			}
			window->data[i].size += ret;
		}
		write(window->sockfd, END_OF_COMMAND_LINE, 2);
	}

	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	//	printf("redis_op_hmset buffer = |%s|\n", buffer);
	p = buffer;
	if (strlen(p) < 5) {
		if (*p == '+') {
			p++;
			tmp_p = p;
			while (*tmp_p != '\r' && *tmp_p != '\n') {
				tmp_p++;
			}
			tmp_p = '\0';

			ret = strncasecmp(p, "OK", strlen("OK"));
		} else {
			ret = -1;
		}
	}

	return ret;
}


int	redis_op_hexists(struct redis_window *window, char *field)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	char			*tmp_p = NULL;
	int			ret = -1;

	p = buffer;
	merge_hexists_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = merge_field_name(p, field);
	write(window->sockfd, p, ret);

	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		tmp_p = p;
		while (*tmp_p != '\r' && *tmp_p != '\n') {
			tmp_p++;
		}
		tmp_p = '\0';
		ret =atoi(p);
	} else {
		ret = -1;
	}

	return ret;
}


int	redis_op_set(struct redis_window *window, char *value, int value_len)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	int			ret = -1;

	p = buffer;
	merge_set_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = merge_value_len(p, value_len);
	write(window->sockfd, p, ret);

	while ((ret = write(window->sockfd, value, value_len) > 0)) {
		if (ret < value_len) {
			break;
		}
		value += ret;
	}
	write(window->sockfd, END_OF_COMMAND_LINE, 2);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == '+') {
		p++;
		return strncasecmp(p, "OK", strlen("OK"));
	} else {
		ret = -1;
	}

	return ret;
}


int	redis_op_hget(struct redis_window *window, char *dest, char *field, int *skip)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	int			ret = 0;
	int			i = 0;
	int			count = 0;
	char			value_size_str[32] = {0, };
	int			value_size = 0;


	p = buffer;
	merge_hget_method_opt(p);
	merge_key_name(p, window->keyname);
	ret = merge_field_name(p, field);
	ret = write(window->sockfd, p, ret);

	ret = 0;
	p = dest;
	//	memset(buffer, 0, sizeof(buffer));
	while ((count = read(window->sockfd, buffer, sizeof(buffer))) > 0) {
		memcpy(p, buffer, count);
		if (ret == 0) {
			ret += count;
			/*the 0x24 ascii is $ */
			printf("*p = %x\n", *p);
			if (*p == 0x24) {
				p++;
				for (i = 0; (*p != '\r') && (*p != '\n'); i++, p++) {
					value_size_str[i] = *p;
				}
			}
			value_size = atoi(value_size_str);
			printf("%s value_size = %d\n", __func__, value_size);
			p += 2;
			/*because it move pointer three char space use p
			 * so is add 3, and loop i times*/
			*skip = 3 + i;
			count = count - 3 - i;

		} else {
			ret += count;
		}
		printf("count = %d, ret = %d\n", count ,ret);
		/*skip of the '$' '\r' '\n' '\r' '\n' 5 character */
		p += count;
		if (ret == value_size + strlen(value_size_str) + 5) break;
	}
	printf("value_size = %d\n", value_size);
	return value_size;
}



int	redis_op_hmget(struct redis_window *window, char *dest, int *skip, int field_num)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	int			ret = 0;
	int			i = 0;
	int			count = 0;
	char			value_size_str[32] = {0, };
	int			value_size = 0;
	int			tmp_size = 0;



	p = buffer;
	merge_hmget_method_opt(p, field_num);
	merge_key_name(p, window->keyname);

	for (i = 0; i < field_num; i++) {
		ret = merge_field_name(p, window->data[i].field);
	}
	ret = strlen(p);
	write(window->sockfd, p, ret);

	ret = 0;
	p = dest;
	memset(buffer, 0, sizeof(buffer));
	while ((count = read(window->sockfd, buffer, sizeof(buffer))) > 0) {
		printf("read buffer = %s\n", buffer);
		memcpy(p, buffer, count);
		if (ret == 0) {
			ret += count;
			/*the 0x24 ascii is $ */
			if (*p == '*') {
				p++;
				for (i = 0; (*p != '\r') && (*p != '\n'); i++, p++) {
					value_size_str[i] = *p;
				}
			}
			value_size = atoi(value_size_str);
			tmp_size = value_size;
			p += 2;
			/*because it move pointer three char space use p
			 * so is add 3, and loop i times*/
			*skip = 3 + i;
			count = count - 3 - i;
		} else {
			ret += count;
		}

		printf("tmp_size = %d\n", tmp_size);
		for (i = 0; tmp_size >= 0; p++, i++) {
			if (*p != '\n') {
				tmp_size--;
			}
			if (!tmp_size) {
				goto out;
			}
		}

		/*skip of the '$' '\r' '\n' '\r' '\n' 5 character */
		p += count;
		if (ret == value_size + strlen(value_size_str) + 5) break;
	}
out:
	return value_size;
}


int	redis_op_hdel(struct redis_window *window, char *dest, int *skip, int field_num)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	int			ret = 0;
	int			i = 0;
	int			count = 0;
	char			value_size_str[32] = {0, };
	int			value_size = 0;
	int			tmp_size = 0;



	p = buffer;
	merge_hdel_method_opt(p, field_num);
	merge_key_name(p, window->keyname);

	for (i = 0; i < field_num; i++) {
		ret = merge_field_name(p, window->data[i].field);
	}
	ret = strlen(p);
	write(window->sockfd, p, ret);


	ret = 0;
	p = dest;
	memset(buffer, 0, sizeof(buffer));
	while ((count = read(window->sockfd, buffer, sizeof(buffer))) > 0) {
	//	printf("read buffer = %s\n", buffer);
		memcpy(p, buffer, count);
		if (ret == 0) {
			ret += count;
			/*the 0x24 ascii is $ */
			if (*p == '*') {
				p++;
				for (i = 0; (*p != '\r') && (*p != '\n'); i++, p++) {
					value_size_str[i] = *p;
				}
			} else if (*p == ':') {
				p++;
				for (i = 0; (*p != '\r') && (*p != '\n'); i++, p++) {
					value_size_str[i] = *p;
				}
			}

			value_size = atoi(value_size_str);
			tmp_size = value_size;
			p += 2;
			/*because it move pointer three char space use p
			 * so is add 3, and loop i times*/
			*skip = 3 + i;
			count = count - 3 - i;
		} else {
			ret += count;
		}

		if (tmp_size == 0) goto out;
		//	printf("tmp_size = %d\n", tmp_size);
		for (i = 0; tmp_size >= 0; p++, i++) {
			if (*p != '\n') {
				tmp_size--;
			}
			if (!tmp_size) {
				goto out;
			}
		}

		/*skip of the '$' '\r' '\n' '\r' '\n' 5 character */
		p += count;
		if (ret == value_size + strlen(value_size_str) + 5) break;
	}
out:
	return value_size;
}


int	redis_op_hgetall(struct redis_window *window, char *dest, int *skip)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	int			ret = 0;
	int			i = 0;
	int			count = 0;
	char			value_size_str[32] = {0, };
	int			value_size = 0;
	int			tmp_size = 0;


	p = buffer;
	merge_hgetall_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	write(window->sockfd, p, ret);

	ret = 0;
	p = dest;
	memset(buffer, 0, sizeof(buffer));
	while ((count = read(window->sockfd, buffer, sizeof(buffer))) > 0) {
		memcpy(p, buffer, count);
		if (ret == 0) {
			ret += count;
			/*the 0x24 ascii is $ */
			if (*p == '*') {
				p++;
				for (i = 0; (*p != '\r') && (*p != '\n'); i++, p++) {
					value_size_str[i] = *p;
				}
			}
			value_size = atoi(value_size_str);
			tmp_size = value_size;
			p += 2;
			/*because it move pointer three char space use p
			 * so is add 3, and loop i times*/
			*skip = 3 + i;
			count = count - 3 - i;
		} else {
			ret += count;
		}

		for (i = 0; tmp_size >= 0; p++, i++) {
			if (*p != '\n') {
				tmp_size--;
			}
			if (!tmp_size) {
				goto out;
			}
		}
		/*skip of the '$' '\r' '\n' '\r' '\n' 5 character */
		p += count;
		if (ret == value_size + strlen(value_size_str) + 5) break;
	}
out:
	return value_size;
}

int	redis_op_hvals(struct redis_window *window, char *dest, int *skip)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	int			ret = 0;
	int			i = 0;
	int			count = 0;
	char			value_size_str[32] = {0, };
	int			value_size = 0;
	int			tmp_size = 0;


	p = buffer;
	merge_hvals_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	write(window->sockfd, p, ret);

	ret = 0;
	p = dest;
	memset(buffer, 0, sizeof(buffer));
	while ((count = read(window->sockfd, buffer, sizeof(buffer))) > 0) {
		memcpy(p, buffer, count);
		if (ret == 0) {
			ret += count;
			/*the 0x24 ascii is $ */
			if (*p == '*') {
				p++;
				for (i = 0; (*p != '\r') && (*p != '\n'); i++, p++) {
					value_size_str[i] = *p;
				}
			}
			value_size = atoi(value_size_str);
			tmp_size = value_size;
			p += 2;
			/*because it move pointer three char space use p
			 * so is add 3, and loop i times*/
			*skip = 3 + i;
			count = count - 3 - i;
		} else {
			ret += count;
		}

		for (i = 0; tmp_size >= 0; p++, i++) {
			if (*p != '\n') {
				tmp_size--;
			}
			if (!tmp_size) {
				goto out;
			}
		}
		/*skip of the '$' '\r' '\n' '\r' '\n' 5 character */
		p += count;
		if (ret == value_size + strlen(value_size_str) + 5) break;
	}
out:
	return value_size;
}


int	redis_op_hkeys(struct redis_window *window, char *dest, int *skip)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			*p = NULL;
	int			ret = 0;
	int			i = 0;
	int			count = 0;
	char			value_size_str[32] = {0, };
	int			value_size = 0;
	int			tmp_size = 0;


	p = buffer;
	merge_hkeys_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	write(window->sockfd, p, ret);

	ret = 0;
	p = dest;
	memset(buffer, 0, sizeof(buffer));
	while ((count = read(window->sockfd, buffer, sizeof(buffer))) > 0) {
		memcpy(p, buffer, count);
		if (ret == 0) {
			ret += count;
			/*the 0x24 ascii is $ */
			if (*p == '*') {
				p++;
				for (i = 0; (*p != '\r') && (*p != '\n'); i++, p++) {
					value_size_str[i] = *p;
				}
			}
			value_size = atoi(value_size_str);
			tmp_size = value_size;
			p += 2;
			/*because it move pointer three char space use p
			 * so is add 3, and loop i times*/
			*skip = 3 + i;
			count = count - 3 - i;
		} else {
			ret += count;
		}

		for (i = 0; tmp_size >= 0; p++, i++) {
			if (*p != '\n') {
				tmp_size--;
			}
			if (!tmp_size) {
				goto out;
			}
		}
		/*skip of the '$' '\r' '\n' '\r' '\n' 5 character */
		p += count;
		if (ret == value_size + strlen(value_size_str) + 5) break;
	}
out:
	return value_size;
}


int	redis_op_strlen(struct redis_window *window)
{
	char		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char		strsize[32] = {0, };
	char		*p = NULL;
	int		ret = 0;
	int		i = 0;

	p = buffer;
	ret = merge_strlen_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = write(window->sockfd, p, ret);

	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		while (*p != '\r') {
			strsize[i] = *p;
			p++;
			i++;
		}
	}
	return atoi(strsize);
}


int	redis_op_hlen(struct redis_window *window)
{
	char		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char		strsize[32] = {0, };
	char		*p = NULL;
	int		ret = 0;
	int		i = 0;

	p = buffer;
	ret = merge_hlen_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = write(window->sockfd, p, ret);

	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		while (*p != '\r') {
			strsize[i] = *p;
			p++;
			i++;
		}
	}
	return atoi(strsize);
}


long long	redis_op_decr(struct redis_window *window)
{
	char		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char		strsize[32] = {0, };
	char		*p = NULL;
	int		ret = 0;
	int		i = 0;

	p = buffer;
	ret = merge_decr_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = write(window->sockfd, p, ret);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		while (*p != '\r') {
			strsize[i] = *p;
			p++;
			i++;
		}
	}
	return atoll(strsize);
}

int	redis_op_del(struct redis_window *window)
{
	char		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char		*p = NULL;
	char		*cmp_str = "OK";
	int		ret = 0;
	int		i = 0;

	p = buffer;
	ret = merge_del_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = write(window->sockfd, p, ret);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	i = strlen(cmp_str);
	if (*p == '+') {
		p++;
		ret = strncasecmp(p, cmp_str,i);
		return ret;
	}
	return ret;
}


long long	redis_op_hincrby(struct redis_window *window)
{
	char		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char		strsize[32] = {0, };
	char		*p = NULL;
	int		ret = 0;
	int		i = 0;

	p = buffer;
	ret = merge_hincrby_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = merge_field_name(p, window->data[FILE_CONTEXT].field);
	ret = merge_int_value(p, window->data[FILE_CONTEXT].size);
	ret = write(window->sockfd, p, ret);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		while (*p != '\r') {
			strsize[i] = *p;
			p++;
			i++;
		}
	}

	return atoll(strsize);
}


long long	redis_op_incr(struct redis_window *window)
{
	char		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char		strsize[32] = {0, };
	char		*p = NULL;
	int		ret = 0;
	int		i = 0;

	p = buffer;
	ret = merge_incr_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = write(window->sockfd, p, ret);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		while (*p != '\r') {
			strsize[i] = *p;
			p++;
			i++;
		}
	}
	return atoll(strsize);
}


int	redis_op_zadd(struct redis_window *window, char *value, int score, int value_len)
{
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			score_buf[64] = {0, };
	char			*p = NULL;
	char			*tmp_p = NULL;
	int			ret = -1;

	p = buffer;
	merge_zadd_method_opt(p);
	ret = merge_key_name(p, window->keyname);
	ret = merge_int_value(p, score);
	ret = merge_value_len(p, value_len);
	write(window->sockfd, p, ret);

	printf("|%s|\n", p);
	while ((ret = write(window->sockfd, value, value_len) > 0)) {
		if (ret < value_len) {
			break;
		}
		value += ret;
	}
	write(window->sockfd, END_OF_COMMAND_LINE, 2);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == ':') {
		p++;
		tmp_p = p;
		while (*tmp_p != '\r' && *tmp_p != '\n') {
			tmp_p++;
		}
		tmp_p = '\0';
		ret =atoi(p);
	} else {
		ret = -1;
	}

	return ret;
}

int	redis_op_ping(struct redis_window *window)
{
	char		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char		*p = NULL;
	int		ret = 0;

	p = buffer;
	ret = merge_data(p, "ping");
	ret = write(window->sockfd, p, ret);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == '+') {
		p++;
		ret = strncasecmp(p, "PONG", strlen("PONG"));
	}
	return ret;
}

int	redis_op_quit(struct redis_window *window)
{
	char		buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char		*p = NULL;
	int		ret = 0;

	p = buffer;
	ret = merge_data(p, "quit");
	ret = write(window->sockfd, p, ret);
	memset(buffer, 0, sizeof(buffer));
	ret = read(window->sockfd, buffer, sizeof(buffer));
	p = buffer;
	if (*p == '+') {
		p++;
		ret = strncasecmp(p, "OK", strlen("OK"));
	}
	return ret;
}


int	new_redis_window(struct redis_window *window)
{
	struct sockaddr_in	addr;

	window->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (window->sockfd < 0) {
		printf("create socket error\n");
		return -1;
	}
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(window->port);
	addr.sin_addr.s_addr = inet_addr(window->ipaddr);
	if (connect(window->sockfd, (const struct sockaddr *)&addr,
				sizeof(addr)) < 0) {
		printf("connect error\n");
		return -1;
	}

	return 0;
}

void delete_redis_window(struct redis_window *window)
{
	if (window && window->sockfd) {
		shutdown(window->sockfd, SHUT_RDWR);
		close(window->sockfd);
		memset(window, 0, sizeof(struct redis_window));
	}
}

#if 0
void start_send_file(char *ipaddr, int port, char *filename, char *keyname)
{
	int			sockfd = 0;
	int			flag = 0;
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0, };
	char			cmd_buffer[MAX_SIZE_OF_VALUE_BUFFER] = {0,};
	struct sockaddr_in	addr;
	int			fd = 0;
	long long		ret = 0;
	int			localfd = 0;
	int			count = 0;
	int			skip = 0;
	int			i = 0;
	char			*p = NULL;
	struct stat 		sb;
	char			*test_field[20];
	struct field_value	test_field_value[32];
	memset(buffer, 0, sizeof(buffer));
	p = buffer;
	memset(&sb, 0, sizeof(sb));
	bzero(&addr, sizeof(addr));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	g_sockfd = sockfd;
	if (sockfd < 0) {
		perror("socket");
		exit (0);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ipaddr);


	flag = fcntl(sockfd, F_GETFL);


	if (connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		exit(1);
	}

#if 1
	//set test
	if (stat(filename, &sb) == -1) {
		perror("stat");
	}

	fd = open(filename, O_RDONLY);
	localfd = open("asd.jpg", O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
	memset(buffer, 0, sizeof(buffer));
	p = cmd_buffer;
	while ((ret = read(fd, p, 1024))) {
		p += 1024;
	}
#if 0
	redis_op_set(keyname, buffer, sb.st_size);
	memset(buffer, 0, sizeof(buffer));
	ret = get_value_by_key(sockfd, buffer, keyname, &skip);
	p = buffer;
	p += skip;
	while (((count = write(localfd, p, ret)) > 0) && ret) {
		ret -= count;
	}
	// end test

	//append test
	redis_op_append(keyname, "fuckfuckfuckfuck", strlen("fuckfuckfuckfuck"));
	//end test

	ret = redis_op_strlen(keyname);
#else

	//test for proj
	redis_op_del(keyname);
	memset(buffer, 0 ,sizeof(buffer));
	p = cmd_buffer;
	for (i = 0; i < 2; i++) {
		memset (buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "field%d", i);
		redis_op_hset(keyname, p, buffer, sb.st_size);
	}

	for (i = 0; i < 2; i++) {
		memset (buffer, 0, sizeof(buffer));
		memset (cmd_buffer, 0, sizeof(cmd_buffer));
		snprintf(buffer, sizeof(buffer), "field%d", i);
		ret = redis_op_hget(keyname, cmd_buffer, buffer, &skip);
		p = cmd_buffer;
		p += skip;
		while (((count = write(localfd, p, ret)) > 0) && ret) {
			ret -= count;
		}
//		memset(buffer, 0, sizeof(buffer));
//		snprintf(buffer, ret, "%s", p);
	}

	//end test
#endif	
	// test decr
	ret = merge_set_method_opt(p);
	ret = merge_key_name(p, keyname);
	ret = merge_value_len(p, strlen("92233723"));
	write(sockfd, p, ret);
	write(sockfd, "92233723", strlen("92233723"));
	write(sockfd, END_OF_COMMAND_LINE, 2);
	memset(cmd_buffer, 0, sizeof(cmd_buffer));
	ret = read(sockfd, cmd_buffer, 32);
	ret = redis_op_decr(keyname);
	//end test
#endif	
	// test incr
	redis_op_del(keyname);
	memset(buffer, 0 ,sizeof(buffer));
	p = buffer;
	ret = merge_set_method_opt(p);
	ret = merge_key_name(p, keyname);
	ret = merge_value_len(p, strlen("92233723"));
	write(sockfd, p, ret);
	write(sockfd, "92233723", strlen("92233723"));
	write(sockfd, END_OF_COMMAND_LINE, 2);
	memset(cmd_buffer, 0, sizeof(cmd_buffer));
	ret = read(sockfd, cmd_buffer, 32);
	ret = redis_op_incr(keyname);
	//end test

	//ping test
	redis_op_ping();
	//end test


	//hset test

	redis_op_del(keyname);
	memset(buffer, 0 ,sizeof(buffer));
	p = buffer;
	for (i = 0; i < 10; i++) {
		memset (buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "field%d", i);
		redis_op_hset(keyname, "hello", buffer, strlen("hello"));
	}
	//end test

	//hget test
	for (i = 0; i < 10; i++) {
		memset (buffer, 0, sizeof(buffer));
		memset (cmd_buffer, 0, sizeof(cmd_buffer));
		snprintf(buffer, sizeof(buffer), "field%d", i);
		ret = redis_op_hget(keyname, cmd_buffer, buffer, &skip);
		p = cmd_buffer;
		p += skip;
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, ret, "%s", p);
	}
	//end test


	//hgetall test
	memset(cmd_buffer, 0, sizeof(cmd_buffer));
	redis_op_hgetall(keyname, cmd_buffer, &skip);
	//end test



	//hkeys test
	memset(cmd_buffer, 0, sizeof(cmd_buffer));
	redis_op_hkeys(keyname, cmd_buffer, &skip);
	//end test


	//hlen test
	ret = redis_op_hlen(keyname);
	printf("the hlen = %lld\n", ret);
	//end test


	//hvals test
	memset(cmd_buffer, 0, sizeof(cmd_buffer));
	redis_op_hvals(keyname, cmd_buffer, &skip);
	//end test

	//hexists test
	for (i = 0; i < 15; i++) {
		memset (buffer, 0, sizeof(buffer));
		memset (cmd_buffer, 0, sizeof(cmd_buffer));
		snprintf(buffer, sizeof(buffer), "field%d", i);
		ret = redis_op_hexists(keyname, buffer);
	//	printf("the file = |%s| exists = %lld\n", buffer, ret);
	}

	//end test

	//hmset test
	memset (cmd_buffer, 0, sizeof(cmd_buffer));
	memset (buffer, 0, sizeof(buffer));
	for (i = 0; i < 20; i++) {
		snprintf(test_field_value[i].field, sizeof(buffer), "field%d", i);
		snprintf(test_field_value[i].value, sizeof(buffer), "valuefuck%d", i);
		test_field_value[i].size = strlen(test_field_value[i].value);
	}

	redis_op_hmset(NULL, test_field_value, i);
	//end test

	//hmget test
	memset (cmd_buffer, 0, sizeof(cmd_buffer));
	memset (buffer, 0, sizeof(buffer));
	for (i = 0; i < 20; i++) {
		test_field[i] = malloc(32);
		snprintf(test_field[i], sizeof(buffer), "field%d", i);
	}
	redis_op_hmget(keyname, cmd_buffer, test_field, &skip, i);
	for (i = 0; i < 20; i++) {
		 free(test_field[i]);
	}

	//end test

	//hdel test
	memset (cmd_buffer, 0, sizeof(cmd_buffer));
	memset (buffer, 0, sizeof(buffer));
	for (i = 0; i < 20; i++) {
		test_field[i] = malloc(32);
		snprintf(test_field[i], sizeof(buffer), "field%d", i);
	}
	redis_op_hdel(keyname, cmd_buffer, test_field, &skip, i);
	for (i = 0; i < 20; i++) {
		 free(test_field[i]);
	}

	//end test



	//hmget test
	memset (cmd_buffer, 0, sizeof(cmd_buffer));
	memset (buffer, 0, sizeof(buffer));
	for (i = 0; i < 20; i++) {
		test_field[i] = malloc(32);
		snprintf(test_field[i], sizeof(buffer), "field%d", i);
	}
	redis_op_hmget(keyname, cmd_buffer, test_field, &skip, i);
	for (i = 0; i < 20; i++) {
		 free(test_field[i]);
	}

	//end test

	//hsetnx test
	for (i = 0; i < 10; i++) {
		memset (buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "field%d", i);
		redis_op_hsetnx(keyname, "hello", buffer, strlen("hello"));
	}

	//end test
	//hmget test
	memset (cmd_buffer, 0, sizeof(cmd_buffer));
	memset (buffer, 0, sizeof(buffer));
	for (i = 0; i < 20; i++) {
		test_field[i] = malloc(32);
		snprintf(test_field[i], sizeof(buffer), "field%d", i);
	}
	redis_op_hmget(keyname, cmd_buffer, test_field, &skip, i);
	for (i = 0; i < 20; i++) {
		 free(test_field[i]);
	}

	//end test


	//hincrby test
	redis_op_del(keyname);
	memset (cmd_buffer, 0, sizeof(cmd_buffer));
	memset (buffer, 0, sizeof(buffer));
	for (i = 0; i < 20; i++) {
		memset (&test_field_value[i], 0, sizeof(test_field_value[i]));
		snprintf(test_field_value[i].field, sizeof(buffer), "field%d", i);
		snprintf(test_field_value[i].value, sizeof(buffer), "%d", i+10);
		test_field_value[i].size = strlen(test_field_value[i].value);
	}

	redis_op_hmset(NULL, test_field_value, i);

	redis_op_hincrby(keyname, test_field_value[0].field, 5);
	//end test





	close(localfd);
	close(fd);
	shutdown(localfd, SHUT_RDWR);
}

#endif
void test_func(char *ipaddr, int port, char *filename, char *keyname)
{
	int 			ret = 0;
	int			fd = 0;
	int			count = 0;
	int			localfd = 0;
	int			skip = 0;
	char			*p = NULL;
	char			buffer[MAX_SIZE_OF_VALUE_BUFFER * 2] = {0, };
	struct redis_window 	window;
	struct stat 		sb;


	memset(&window, 0, sizeof(struct redis_window));

	strncpy(window.ipaddr, ipaddr, strlen(ipaddr));
	window.port = port;
	strncpy(window.keyname, filename, strlen(filename));
	strncpy(window.keyname, filename, strlen(filename));
	ret = new_redis_window(&window);
	
	if (stat(filename, &sb) == -1) {
		printf("stat %s error\n", filename);
	}

	snprintf(window.data[FILE_STAT].field, 
			sizeof(window.data[FILE_STAT].field), "stat");
	snprintf(window.data[FILE_STAT].value,
			sizeof(window.data[FILE_STAT].value), "%ld", sb.st_size);
	window.data[FILE_STAT].size = strlen(window.data[FILE_STAT].value);
	snprintf(window.data[FILE_CONTEXT].field,
			sizeof(window.data[FILE_CONTEXT].field), "context");
	snprintf(window.data[FILE_NEW_DATA].field,
			sizeof(window.data[FILE_NEW_DATA].field), "newdata");
	
	ret = redis_op_hexists(&window, "newdata");
	printf("========----ret = %d----=======\n", ret);

	memset(window.keyname, 0, sizeof(window.keyname));
	strncpy(window.keyname, SORT_KEYNAME, strlen(SORT_KEYNAME));
	redis_op_zadd(&window, "test", 222, strlen("test"));
#if 0
	fd = open(filename, O_RDONLY);
	localfd = open("asd.jpg", O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
//	memset(, 0, sizeof(buffer));
	p = window.data[FILE_CONTEXT].value;
	
	while ((ret = read(fd, p, 1024))) {
		p += 1024;
	}
	window.data[FILE_CONTEXT].size = sb.st_size;

	printf("the window.data[FILE_CONTEXT].size = %lld\n", 
			window.data[FILE_CONTEXT].size);

	printf("window sockfd = %x\n", window.sockfd);
	redis_op_hmset(&window, 2);
	printf("===set after\n");
	ret = redis_op_hget(&window, buffer, 
			window.data[FILE_CONTEXT].field, 
			&skip);
	p = buffer;
	p += skip;
	while (((count = write(localfd, p, ret)) > 0) && ret) {
		ret -= count;
	}
	close(localfd);
	close(fd);

	ret = redis_op_hexists(&window, "newdata");
	
	printf("========----ret = %d----=======\n", ret);

	memset(buffer, 0, sizeof(buffer));
	p = buffer;
//	redis_op_hdel(&window, p, &skip, 2);

	ret = redis_op_hexists(&window, "newdata");
	printf("========----ret = %d----=======\n", ret);
#endif
	redis_op_quit(&window);
}

void usage(void)
{
	fprintf(stderr, "./test [-h ipaddr] [-p port] <-f file path> <-k set keyname>\n");
}

int main(int argc, char *argv[])
{
	int	ret = -1;
	int	opt = 0;
	int	port = 12365;
	char	filename[512] = {0, };
	char	ipaddr[32] = {0, };
	char	keyname[32] = {0, };
	fprintf(stdout, "In programe\n");

	strncpy(ipaddr, "127.0.0.1", strlen("127.0.0.1"));

	if (argc < 2) {
		usage();
		return ret;
	}

	while ((opt = getopt(argc, argv, "Hh:p:f:k:")) != -1) {
		switch (opt) {
			case 'h':
				memset(ipaddr, 0, sizeof(ipaddr));
				strncpy(ipaddr, optarg, strlen(optarg));
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'f':
				memset(filename, 0, sizeof(filename));
				strncpy(filename, optarg, strlen(optarg));
				break;
			case 'k':
				memset(keyname, 0, sizeof(keyname));
				strncpy(keyname, optarg, strlen(optarg));
				break;
			default:
				usage();
				break;
		}
	}


	test_func(ipaddr, port, filename, keyname);

//	start_send_file(ipaddr, port, filename, keyname);
	fprintf(stdout, "Out programe\n");
	return ret;
}
