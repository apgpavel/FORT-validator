#include "address.h"

#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> /* inet_ntop */

static char const *
addr2str4(struct in_addr *addr, char *buffer)
{
	return inet_ntop(AF_INET, addr, buffer, INET_ADDRSTRLEN);
}

static char const *
addr2str6(struct in6_addr *addr, char *buffer)
{
	return inet_ntop(AF_INET6, addr, buffer, INET6_ADDRSTRLEN);
}

static int const
str2addr4(const char *addr, struct in_addr *dst)
{
	if (!inet_pton(AF_INET, addr, dst))
		return -EINVAL;
	return 0;
}

static int const
str2addr6(const char *addr, struct in6_addr *dst)
{
	if (!inet_pton(AF_INET6, addr, dst))
		return -EINVAL;
	return 0;
}

/*
 * Returns a mask you can use to extract the suffix bits of a 32-bit unsigned
 * number whose prefix lengths @prefix_len.
 * For example: Suppose that your number is 192.0.2.0/24.
 * u32_suffix_mask(24) returns 0.0.0.255.
 *
 * The result is in host byte order.
 */
static uint32_t
u32_suffix_mask(unsigned int prefix_len)
{
	/* `a >> 32` is undefined if `a` is 32 bits. */
	return (prefix_len < 32) ? (0xFFFFFFFFu >> prefix_len) : 0;
}

/**
 * Same as u32_suffix_mask(), except the result is in network byte order
 * ("be", for "big endian").
 */
static uint32_t
be32_suffix_mask(unsigned int prefix_len)
{
	return htonl(u32_suffix_mask(prefix_len));
}

static void
ipv6_suffix_mask(unsigned int prefix_len, struct in6_addr *result)
{
	if (prefix_len < 32) {
		result->s6_addr32[0] |= be32_suffix_mask(prefix_len);
		result->s6_addr32[1] = 0xFFFFFFFFu;
		result->s6_addr32[2] = 0xFFFFFFFFu;
		result->s6_addr32[3] = 0xFFFFFFFFu;
	} else if (prefix_len < 64) {
		result->s6_addr32[1] |= be32_suffix_mask(prefix_len - 32);
		result->s6_addr32[2] = 0xFFFFFFFFu;
		result->s6_addr32[3] = 0xFFFFFFFFu;
	} else if (prefix_len < 96) {
		result->s6_addr32[2] |= be32_suffix_mask(prefix_len - 64);
		result->s6_addr32[3] = 0xFFFFFFFFu;
	} else {
		result->s6_addr32[3] |= be32_suffix_mask(prefix_len - 96);
	}
}

int
prefix4_decode(const char *str, struct ipv4_prefix *result)
{
	int error;

	if (str == NULL) {
		warnx("Null string received, can't decode IPv4 prefix");
		return -EINVAL;
	}

	error = str2addr4(str, &result->addr);
	if (error) {
		warnx("Invalid IPv4 prefix '%s'", str);
		return error;
	}

	return 0;
}

int
prefix6_decode(const char *str, struct ipv6_prefix *result)
{
	int error;

	/*
	 * TODO (review) I see this pattern often: You call `err()`, and then
	 * throw a pretty exception.
	 *
	 * `err()` does not return, so any cleanup that follows it is pointless.
	 *
	 * More importantly, returnless functions are bad practice,
	 * because they destroy the recovery potential of calling code.
	 * Use `warn()`/`warnx()` instead (and continue throwing cleanly).
	 */
	if (str == NULL) {
		warnx("Null string received, can't decode IPv6 prefix");
		return -EINVAL;
	}

	error = str2addr6(str, &result->addr);
	if (error) {
		warnx("Invalid IPv6 prefix '%s'", str);
		return error;
	}

	return 0;
 }

int
prefix_length_decode (const char *text, unsigned int *dst, int max_value)
{
	unsigned long len;

	/*
	 * TODO (review) You probably meant to use `errx()`, not `err()`.
	 *
	 * `err()` depends on a thread variable called `errno`. It makes no
	 * sense to call `err` when you know `errno` has not been set properly
	 * by some system function.
	 *
	 * If you haven't done it, see `man 3 err` and `man 3 errno`.
	 */
	if (text == NULL) {
		warnx("Null string received, can't decode prefix length");
		return -EINVAL;
	}

	errno = 0;
	len = strtoul(text, NULL, 10);
	if (errno) {
		warn("Invalid prefix length '%s': %s", text,
		    strerror(errno));
		return -EINVAL;
	}
	/* An underflow or overflow will be considered here */
	if (len < 0 || max_value < len) {
		warnx("Prefix length (%ld) is out of bounds (0-%d).",
		    len, max_value);
		return -EINVAL;
	}
	*dst = (unsigned int) len;
	return 0;
}

int
prefix4_validate (struct ipv4_prefix *prefix)
{
	char buffer[INET_ADDRSTRLEN];

	if ((prefix->addr.s_addr & be32_suffix_mask(prefix->len)) != 0) {
		warn("IPv4 prefix %s/%u has enabled suffix bits.",
		    addr2str4(&prefix->addr, buffer), prefix->len);
		return -EINVAL;
	}
	return 0;
}

int
prefix6_validate (struct ipv6_prefix *prefix)
{
	struct in6_addr suffix;
	char buffer[INET6_ADDRSTRLEN];

	memset(&suffix, 0, sizeof(suffix));
	ipv6_suffix_mask(prefix->len, &suffix);
	if ((prefix->addr.s6_addr32[0] & suffix.s6_addr32[0])
	    || (prefix->addr.s6_addr32[1] & suffix.s6_addr32[1])
	    || (prefix->addr.s6_addr32[2] & suffix.s6_addr32[2])
	    || (prefix->addr.s6_addr32[3] & suffix.s6_addr32[3])) {
		warn("IPv6 prefix %s/%u has enabled suffix bits.",
		    addr2str6(&prefix->addr, buffer), prefix->len);
		return -EINVAL;
	}
	return 0;
}
