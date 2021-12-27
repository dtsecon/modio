/**
 * Modbus input output access tool
 */

#ifndef MXIO_H
#define MXIO_H

#ifndef REGISTER_PATH
#define REGISTER_PATH "/usr/local/share/modio/"
#endif

#define DEVICE_PATH "/dev/ttyUSB0"
#define BAUD_RATE 9600
#define DATA_BIT 8
#define PARITY 'N'
#define STOP_BIT 1
#define MODBUS_SLAVE_ID 1
#define REG_SIZE 64

/* definition of register type */
enum regtype {
    COIL = 0,
    INPUT_B = 1,
    INPUT_R = 2,
    HOLDING = 3
};
typedef enum regtype regtype_t;

/* definition of serial configuration type */
struct serconf {
    int baud;                   /* baud rate */
    char prty;                  /* parity */
    int sbit;                   /* stop bit */
    int dbit;                   /* data bits */
};
typedef struct serconf serconf_t;

/* read register */
struct rreg {
    int reg;                    /* register number */
    int raddr;                  /* register address */
    int xaddr;                  /* register hex address */
    regtype_t rtype;            /* register type */
};
typedef struct rreg rreg_t;

/* device register struct */
struct dreg {
    int num;                    /* register number */
    int addr;                   /* register address */
    int len;                    /* register length */
    int type;                   /* register type */
    char *name;                 /* register name */
    char *desc;                 /* register description */
    char *range;                /* register range */
    double scale;               /* register scale */
    char *engu;                 /* register engineering unit */
    char *acc;                  /* register access */
    int prfmt;                  /* register print format */
};
typedef struct dreg dreg_t;

/* device list struct */
struct dvlst {
    char *manfc;                /* device manufacturer */
    char *type;                 /* device type */
    char *model;                /* device model */
    int zba;                    /* zero based addressing */
    int nor;                    /* number of registers */
    dreg_t *regs;               /* register list */
};
typedef struct dvlst dvlist_t;

/* register print format */
enum prfmt {
   BIN = 0,                     /* binary format */
   HEX = 1,                     /* hex format */
   DEC = 2,                     /* decimal format */
   ASC = 3,                     /* ASCII format */
   BFD = 4,                     /* decimal byte dot separated */
   BFX = 5,                     /* hex byte dot separated */
   HLO = 6                      /* high / low register word */
};
typedef enum prfmt prfmt_t;

#endif