#ifndef _SESSION_H_
#define _SESSION_H_

typedef enum session_type {
	SESSION_FULLY_OFF,
	SESSION_FULLY_ON,
	SESSION_BRIGHTNESS,
	SESSION_BLINK,
	SESSION_TIME,
	SESSION_WAVE,
} session_type_t;

typedef struct session_base {
	int type;
	int (*execute)(void *context);
} session_base_t;

struct session {
	session_base_t *base;
	struct session *next;
	void *extra;
};
typedef struct session session_t;

typedef struct session_broker {
	session_t *sessions;
	int (*push)(session_t *s);
	session_t *(*pop)(void);
	int (*schedule)(void);
} session_broker_t;

typedef struct exp_link {
	char *key;
	int len;
	session_type_t type;
} exp_link_t;

session_broker_t session_broker;

session_t *session_new(const char *exp);
void session_del(session_t *se);

#endif /* _SESSION_H_ */
