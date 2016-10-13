#include <grub/err.h>
#include <grub/i18n.h>
#include <grub/mm.h>
#include <grub/tpm.h>
#include <grub/misc.h>
#include <grub/i386/pc/int.h>

#define TCPA_MAGIC 0x41504354

static int tpm_presence = -1;

int tpm_present(void);

int tpm_present(void)
{
  struct grub_bios_int_registers regs;

  if (tpm_presence != -1)
    return tpm_presence;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  regs.eax = 0xbb00;
  regs.ebx = TCPA_MAGIC;
  grub_bios_interrupt (0x1a, &regs);

  if (regs.eax == 0)
    tpm_presence = 1;
  else
    tpm_presence = 0;

  return tpm_presence;
}

grub_err_t
grub_tpm_execute(PassThroughToTPM_InputParamBlock *inbuf,
		 PassThroughToTPM_OutputParamBlock *outbuf)
{
  struct grub_bios_int_registers regs;
  grub_addr_t inaddr, outaddr;

  if (!tpm_present())
    return 0;

  inaddr = (grub_addr_t) inbuf;
  outaddr = (grub_addr_t) outbuf;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  regs.eax = 0xbb02;
  regs.ebx = TCPA_MAGIC;
  regs.ecx = 0;
  regs.edx = 0;
  regs.es = (inaddr & 0xffff0000) >> 4;
  regs.edi = inaddr & 0xffff;
  regs.ds = outaddr >> 4;
  regs.esi = outaddr & 0xf;

  grub_bios_interrupt (0x1a, &regs);

  if (regs.eax)
    {
	tpm_presence = 0;
	return grub_error (GRUB_ERR_IO, N_("TPM error %x, disabling TPM"), regs.eax);
    }

  return 0;
}

typedef struct {
	grub_uint32_t pcrindex;
	grub_uint32_t eventtype;
	grub_uint8_t digest[20];
	grub_uint32_t eventdatasize;
	grub_uint8_t event[0];
} GRUB_PACKED Event;

typedef struct {
	grub_uint16_t ipblength;
	grub_uint16_t reserved;
	grub_uint32_t hashdataptr;
	grub_uint32_t hashdatalen;
	grub_uint32_t pcr;
	grub_uint32_t reserved2;
	grub_uint32_t logdataptr;
	grub_uint32_t logdatalen;
} GRUB_PACKED EventIncoming;

typedef struct {
	grub_uint16_t opblength;
	grub_uint16_t reserved;
	grub_uint32_t eventnum;
	grub_uint8_t  hashvalue[20];
} GRUB_PACKED EventOutgoing;

grub_err_t
grub_tpm_log_event(unsigned char *buf, grub_size_t size, grub_uint8_t pcr,
		   const char *description)
{
	struct grub_bios_int_registers regs;
	EventIncoming incoming;
	EventOutgoing outgoing;
	Event *event;
	grub_uint32_t datalength;

	if (!tpm_present())
		return 0;

	datalength = grub_strlen(description);
	event = grub_zalloc(datalength + sizeof(Event));
	if (!event)
		return grub_error (GRUB_ERR_OUT_OF_MEMORY,
				   N_("cannot allocate TPM event buffer"));

	event->pcrindex = pcr;
	event->eventtype = 0x0d;
	event->eventdatasize = grub_strlen(description);
	grub_memcpy(event->event, description, datalength);

	incoming.ipblength = sizeof(incoming);
	incoming.hashdataptr = (grub_uint32_t)buf;
	incoming.hashdatalen = size;
	incoming.pcr = pcr;
	incoming.logdataptr = (grub_uint32_t)event;
	incoming.logdatalen = datalength + sizeof(Event);

	regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
	regs.eax = 0xbb01;
	regs.ebx = TCPA_MAGIC;
	regs.ecx = 0;
	regs.edx = 0;
	regs.es = (((grub_addr_t) &incoming) & 0xffff0000) >> 4;
	regs.edi = ((grub_addr_t) &incoming) & 0xffff;
	regs.ds = (((grub_addr_t) &outgoing) & 0xffff0000) >> 4;
	regs.esi = ((grub_addr_t) &outgoing) & 0xffff;

	grub_bios_interrupt (0x1a, &regs);

	grub_free(event);

	if (regs.eax)
	  {
		tpm_presence = 0;
		return grub_error (GRUB_ERR_IO, N_("TPM error %x, disabling TPM"), regs.eax);
	  }

	return 0;
}
