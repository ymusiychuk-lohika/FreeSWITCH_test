#ifndef __PJSIP_TRANSPORT_MULTIPORT_H__
#define __PJSIP_TRANSPORT_MULTIPORT_H__


/**
 * @file sip_transport_multiport.h
 * @brief 
 * multiport transport
 */


#include <pjsip/sip_transport.h>

/**
 * @defgroup PJSIP_TRANSPORT_MULTIPORT Transport
 * @ingroup PJSIP_TRANSPORT
 * @brief multiport transport
 */

PJ_BEGIN_DECL

/**
 * Create and start datagram multiport transport.
 *
 * @param endpt		The endpoint instance.
 * @param transport	Pointer to receive the transport instance.
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjsip_multiport_start( pjsip_endpoint *endpt, pj_bool_t has_ip6);

PJ_END_DECL

/**
 * @}
 */

#endif	/* __PJSIP_TRANSPORT_MULTIPORT_H__ */

