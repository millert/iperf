/*
 * iperf, Copyright (c) 2014-2018, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include "iperf_config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "iperf.h"
#include "iperf_api.h"

#ifdef __MINGW32__
char *
sockstrerror(int errnum)
{
    static char errbuf[1024];

    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL, errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	errbuf, sizeof(errbuf), NULL)) {
	/* Terminate error string at first newline */
	errbuf[strcspn(errbuf, "\r\n")] = '\0';
    } else {
	/* Unknown error number */
	snprintf(errbuf, sizeof(errbuf), "Error %d");
    }

    return errbuf;
}
#endif /* __MINGW32__ */

/* Do a printf to stderr. */
void
iperf_err(struct iperf_test *test, const char *format, ...)
{
    va_list argp;
    char str[1000];

    va_start(argp, format);
    vsnprintf(str, sizeof(str), format, argp);
    if (test != NULL && test->json_output && test->json_top != NULL)
	cJSON_AddStringToObject(test->json_top, "error", str);
    else
	if (test && test->outfile && test->outfile != stdout) {
	    fprintf(test->outfile, "iperf3: %s\n", str);
	}
	else {
	    fprintf(stderr, "iperf3: %s\n", str);
	}
    va_end(argp);
}

/* Do a printf to stderr or log file as appropriate, then exit. */
void
iperf_errexit(struct iperf_test *test, const char *format, ...)
{
    va_list argp;
    char str[1000];

    va_start(argp, format);
    vsnprintf(str, sizeof(str), format, argp);
    if (test != NULL && test->json_output && test->json_top != NULL) {
	cJSON_AddStringToObject(test->json_top, "error", str);
	iperf_json_finish(test);
    } else
	if (test && test->outfile && test->outfile != stdout) {
	    fprintf(test->outfile, "iperf3: %s\n", str);
	}
	else {
	    fprintf(stderr, "iperf3: %s\n", str);
	}
    va_end(argp);
    if (test)
        iperf_delete_pidfile(test);
    exit(1);
}

int i_errno;

char *
iperf_strerror(int int_errno)
{
    static char errstr[256];
    int len, perr = 0;
    int serr = 0;

    len = sizeof(errstr);
    memset(errstr, 0, len);

    switch (int_errno) {
        case IENONE:
            snprintf(errstr, len, "no error");
            break;
        case IESERVCLIENT:
            snprintf(errstr, len, "cannot be both server and client");
            break;
        case IENOROLE:
            snprintf(errstr, len, "must either be a client (-c) or server (-s)");
            break;
        case IESERVERONLY:
            snprintf(errstr, len, "some option you are trying to set is server only");
            break;
        case IECLIENTONLY:
            snprintf(errstr, len, "some option you are trying to set is client only");
            break;
        case IEDURATION:
            snprintf(errstr, len, "test duration too long (maximum = %d seconds)", MAX_TIME);
            break;
        case IENUMSTREAMS:
            snprintf(errstr, len, "number of parallel streams too large (maximum = %d)", MAX_STREAMS);
            break;
        case IEBLOCKSIZE:
            snprintf(errstr, len, "block size too large (maximum = %d bytes)", MAX_BLOCKSIZE);
            break;
        case IEBUFSIZE:
            snprintf(errstr, len, "socket buffer size too large (maximum = %d bytes)", MAX_TCP_BUFFER);
            break;
        case IEINTERVAL:
            snprintf(errstr, len, "invalid report interval (min = %g, max = %g seconds)", MIN_INTERVAL, MAX_INTERVAL);
            break;
    case IEBIND: /* UNUSED */
            snprintf(errstr, len, "--bind must be specified to use --cport");
            break;
        case IEUDPBLOCKSIZE:
            snprintf(errstr, len, "block size invalid (minimum = %d bytes, maximum = %d bytes)", MIN_UDP_BLOCKSIZE, MAX_UDP_BLOCKSIZE);
            break;
        case IEBADTOS:
            snprintf(errstr, len, "bad TOS value (must be between 0 and 255 inclusive)");
            break;
        case IESETCLIENTAUTH:
             snprintf(errstr, len, "you must specify username (max 20 chars), password (max 20 chars) and a path to a valid public rsa client to be used");
            break;
        case IESETSERVERAUTH:
             snprintf(errstr, len, "you must specify path to a valid private rsa server to be used and a user credential file");
            break;
	case IEBADFORMAT:
	    snprintf(errstr, len, "bad format specifier (valid formats are in the set [kmgtKMGT])");
	    break;
        case IEMSS:
            snprintf(errstr, len, "TCP MSS too large (maximum = %d bytes)", MAX_MSS);
            break;
        case IENOSENDFILE:
            snprintf(errstr, len, "this OS does not support sendfile");
            break;
        case IEOMIT:
            snprintf(errstr, len, "bogus value for --omit");
            break;
        case IEUNIMP:
            snprintf(errstr, len, "an option you are trying to set is not implemented yet");
            break;
        case IEFILE:
            snprintf(errstr, len, "unable to open -F file");
            perr = 1;
            break;
        case IEBURST:
            snprintf(errstr, len, "invalid burst count (maximum = %d)", MAX_BURST);
            break;
        case IEENDCONDITIONS:
            snprintf(errstr, len, "only one test end condition (-t, -n, -k) may be specified");
            break;
	case IELOGFILE:
	    snprintf(errstr, len, "unable to open log file");
	    perr = 1;
	    break;
	case IENOSCTP:
	    snprintf(errstr, len, "no SCTP support available");
	    break;
        case IENEWTEST:
            snprintf(errstr, len, "unable to create a new test");
            perr = 1;
            break;
        case IEINITTEST:
            snprintf(errstr, len, "test initialization failed");
            perr = 1;
            break;
        case IEAUTHTEST:
            snprintf(errstr, len, "test authorization failed");
            break;
        case IELISTEN:
            snprintf(errstr, len, "unable to start listener for connections");
            serr = 1;
            break;
        case IECONNECT:
            snprintf(errstr, len, "unable to connect to server");
            serr = 1;
            break;
        case IEACCEPT:
            snprintf(errstr, len, "unable to accept connection from client");
            serr = 1;
            break;
        case IESENDCOOKIE:
            snprintf(errstr, len, "unable to send cookie to server");
            perr = 1;
            break;
        case IERECVCOOKIE:
            snprintf(errstr, len, "unable to receive cookie at server");
            perr = 1;
            break;
        case IECTRLWRITE:
            snprintf(errstr, len, "unable to write to the control socket");
            serr = 1;
            break;
        case IECTRLREAD:
            snprintf(errstr, len, "unable to read from the control socket");
            serr = 1;
            break;
        case IECTRLCLOSE:
            snprintf(errstr, len, "control socket has closed unexpectedly");
            break;
        case IEMESSAGE:
            snprintf(errstr, len, "received an unknown control message");
            break;
        case IESENDMESSAGE:
            snprintf(errstr, len, "unable to send control message");
            serr = 1;
            break;
        case IERECVMESSAGE:
            snprintf(errstr, len, "unable to receive control message");
            serr = 1;
            break;
        case IESENDPARAMS:
            snprintf(errstr, len, "unable to send parameters to server");
            serr = 1;
            break;
        case IERECVPARAMS:
            snprintf(errstr, len, "unable to receive parameters from client");
            serr = 1;
            break;
        case IEPACKAGERESULTS:
            snprintf(errstr, len, "unable to package results");
            perr = 1;
            break;
        case IESENDRESULTS:
            snprintf(errstr, len, "unable to send results");
            serr = 1;
            break;
        case IERECVRESULTS:
            snprintf(errstr, len, "unable to receive results");
            serr = 1;
            break;
        case IESELECT:
            snprintf(errstr, len, "select failed");
            serr = 1;
            break;
        case IECLIENTTERM:
            snprintf(errstr, len, "the client has terminated");
            break;
        case IESERVERTERM:
            snprintf(errstr, len, "the server has terminated");
            break;
        case IEACCESSDENIED:
            snprintf(errstr, len, "the server is busy running a test. try again later");
            break;
        case IESETNODELAY:
            snprintf(errstr, len, "unable to set TCP/SCTP NODELAY");
            serr = 1;
            break;
        case IESETMSS:
            snprintf(errstr, len, "unable to set TCP/SCTP MSS");
            serr = 1;
            break;
        case IESETBUF:
            snprintf(errstr, len, "unable to set socket buffer size");
            serr = 1;
            break;
        case IESETTOS:
            snprintf(errstr, len, "unable to set IP TOS");
            serr = 1;
            break;
        case IESETCOS:
            snprintf(errstr, len, "unable to set IPv6 traffic class");
            serr = 1;
            break;
        case IESETFLOW:
            snprintf(errstr, len, "unable to set IPv6 flow label");
            break;
        case IEREUSEADDR:
            snprintf(errstr, len, "unable to reuse address on socket");
            serr = 1;
            break;
        case IENONBLOCKING:
            snprintf(errstr, len, "unable to set socket to non-blocking");
            serr = 1;
            break;
        case IESETWINDOWSIZE:
            snprintf(errstr, len, "unable to set socket window size");
            serr = 1;
            break;
        case IEPROTOCOL:
            snprintf(errstr, len, "protocol does not exist");
            break;
        case IEAFFINITY:
            snprintf(errstr, len, "unable to set CPU affinity");
            perr = 1;
            break;
	case IEDAEMON:
	    snprintf(errstr, len, "unable to become a daemon");
	    perr = 1;
	    break;
        case IECREATESTREAM:
            snprintf(errstr, len, "unable to create a new stream");
            perr = 1;
            break;
        case IEINITSTREAM:
            snprintf(errstr, len, "unable to initialize stream");
            serr = 1;
            break;
        case IESTREAMLISTEN:
            snprintf(errstr, len, "unable to start stream listener");
            serr = 1;
            break;
        case IESTREAMCONNECT:
            snprintf(errstr, len, "unable to connect stream");
            serr = 1;
            break;
        case IESTREAMACCEPT:
            snprintf(errstr, len, "unable to accept stream connection");
            serr = 1;
            break;
        case IESTREAMWRITE:
            snprintf(errstr, len, "unable to write to stream socket");
            serr = 1;
            break;
        case IESTREAMREAD:
            snprintf(errstr, len, "unable to read from stream socket");
            serr = 1;
            break;
        case IESTREAMCLOSE:
            snprintf(errstr, len, "stream socket has closed unexpectedly");
            break;
        case IESTREAMID:
            snprintf(errstr, len, "stream has an invalid id");
            break;
        case IENEWTIMER:
            snprintf(errstr, len, "unable to create new timer");
            perr = 1;
            break;
        case IEUPDATETIMER:
            snprintf(errstr, len, "unable to update timer");
            perr = 1;
            break;
        case IESETCONGESTION:
            snprintf(errstr, len, "unable to set TCP_CONGESTION: " 
                                  "Supplied congestion control algorithm not supported on this host");
            break;
	case IEPIDFILE:
            snprintf(errstr, len, "unable to write PID file");
            perr = 1;
            break;
	case IEV6ONLY:
	    snprintf(errstr, len, "Unable to set/reset IPV6_V6ONLY");
	    serr = 1;
	    break;
        case IESETSCTPDISABLEFRAG:
            snprintf(errstr, len, "unable to set SCTP_DISABLE_FRAGMENTS");
            serr = 1;
            break;
        case IESETSCTPNSTREAM:
            snprintf(errstr, len, "unable to set SCTP_INIT num of SCTP streams\n");
            serr = 1;
            break;
	case IESETPACING:
	    snprintf(errstr, len, "unable to set socket pacing");
	    serr = 1;
	    break;
	case IESETBUF2:
	    snprintf(errstr, len, "socket buffer size not set correctly");
	    break;
	
    }

    if (perr || serr)
        strncat(errstr, ": ", len - strlen(errstr) - 1);
    if (perr && errno)
        strncat(errstr, strerror(errno), len - strlen(errstr) - 1);
    else if (serr && sockerrno)
        strncat(errstr, sockstrerror(sockerrno), len - strlen(errstr) - 1);

    return errstr;
}
