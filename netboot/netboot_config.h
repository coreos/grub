/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

extern struct nic	nic;

extern void	print_config(void);
extern void	eth_reset(void);
extern int	eth_probe(void);
extern int	eth_poll(void);
extern void	eth_transmit(char *d, unsigned int t, unsigned int s, char *p);
extern void	eth_disable(void);
