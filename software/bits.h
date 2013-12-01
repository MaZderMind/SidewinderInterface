/**
 * @file
 * Bit-Helper Makros
 *
 * Diese Makros machen das Arbeiten mit Bits und Bitfeldern einfacher
 */
#ifndef BITS_H_
#define BITS_H_

/**
 * Erzeugt einen Integer mit einem einzelnen, gesetzten Bit
 */
#define BIT(x) (1 << (x))
#define BITLONG(x) ((uint64_t)1 << (x))

/**
 * Setzt eine Reihe von Bits in einem Bitfeld
 *
 * Beispiel: SETBITS(PORTB, BIT(PB0) | BIT(PB1));
 */
#define SETBITS(x,y) ((x) |= (y))

/**
 * Löscht eine Reihe von Bits in einem Bitfeld
 *
 * Beispiel: CLEARBITS(PORTB, BIT(PB0) | BIT(PB1));
 */
#define CLEARBITS(x,y) ((x) &= (~(y)))

/**
 * Löscht eine Reihe von Bits in einem Bitfeld
 *
 * Beispiel: CLEARBITS(PORTB, BIT(PB0) | BIT(PB1));
 */
#define TOGGLEBITS(x,y) ((x) ^= (y))

/**
 * Setzt ein einzelnes Bit in einem Bitfeld
 *
 * Beispiel: SETBIT(PORTB, PB0);
 */
#define SETBIT(x,y) SETBITS((x), (BIT((y))))
#define SETBITLONG(x,y) SETBITS((x), (BITLONG((y))))

/**
 * Löscht ein einzelnes Bit in einem Bitfeld
 *
 * Beispiel: CLEARBIT(PORTB, PB0);
 */
#define CLEARBIT(x,y) CLEARBITS((x), (BIT((y))))

/**
 * Löscht ein einzelnes Bit in einem Bitfeld
 *
 * Beispiel: CLEARBIT(PORTB, PB0);
 */
#define TOGGLEBIT(x,y) TOGGLEBITS((x), (BIT((y))))

/**
 * Prüft, ob ein einzelnes Bit in einem Bitfeld gesetzt ist
 *
 * Beispiel: if(BITSET(PINB, PB0)) { ... }
 */
#define BITSET(x,y) ((x) & (BIT(y)))

/**
 * Prüft, ob ein einzelnes Bit in einem Bitfeld gelöscht ist
 *
 * Beispiel: if(BITCLEAR(PINB, PB0)) { ... }
 */
#define BITCLEAR(x,y) !BITSET((x), (y))

/**
 * Prüft, ob eine Reihe von Bits in einem Bitfeld gesetzt ist
 *
 * Beispiel: if(BITSSET(PINB, BIT(PB0) | BIT(PB1))) { ... }
 */
#define BITSSET(x,y) (((x) & (y)) == (y))

/**
 * Prüft, ob eine Reihe von Bits in einem Bitfeld gelöscht ist
 *
 * Beispiel: if(BITSCLEAR(PINB, BIT(PB0) | BIT(PB1))) { ... }
 */
#define BITSCLEAR(x,y) (((x) & (y)) == 0)

/**
 * Unbekannt
 * @todo Rausfinden, was das Makro macht ;)
 */
#define BITVAL(x,y) (((x)>>(y)) & 1)

/**
 * No-Op macro
 */
#define nop() asm volatile("nop")

#endif // BITS_H_
