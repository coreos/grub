/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

/*
 *	Structure returned from eth_probe and passed to other driver
 *	functions.
 */

struct nic
{
	void		(*reset)P((struct nic *));
	int		(*poll)P((struct nic *));
	void		(*transmit)P((struct nic *, char *d,
				unsigned int t, unsigned int s, char *p));
	void		(*disable)P((struct nic *));
	char		aui;
	char		*node_addr;
	char		*packet;
	int		packetlen;
	void		*priv_data;	/* driver can hang private data here */
};
