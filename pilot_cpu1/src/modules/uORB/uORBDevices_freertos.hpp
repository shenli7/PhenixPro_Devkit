/****************************************************************************
 *
 *   Copyright (c) 2012-2015 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef _uORBDevices_nuttx_hpp_
#define _uORBDevices_nuttx_hpp_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uORB/ORBMap.hpp"
#include "uORB/uORBCommon.hpp"


namespace uORB
{
class DeviceNode;
class DeviceMaster;
}

/**
 * Per-object device instance.
 */
class uORB::DeviceNode : public device::CDev
{
public:
	/**
	 * Constructor
	 */
	DeviceNode
	(
		const struct orb_metadata *meta,
		const char *name,
		const char *path,
		int priority
	);

	/**
	 * Destructor
	 */
	~DeviceNode();

	/**
	 * Method to create a subscriber instance and return the struct
	 * pointing to the subscriber as a file pointer.
	 */
	virtual int  open(Handle_t *pHandle);

	/**
	 * Method to close a subscriber for this topic.
	 */
	virtual int   close(Handle_t *pHandle);

	/**
	 * reads data from a subscriber node to the buffer provided.
	 * @param filp
	 *   The subscriber from which the data needs to be read from.
	 * @param buffer
	 *   The buffer into which the data is read into.
	 * @param buflen
	 *   the length of the buffer
	 * @return
	 *   size_t the number of bytes read.
	 */
	virtual size_t  read(Handle_t *pHandle, char *pcBuffer, size_t xBufLen);

	/**
	 * writes the published data to the internal buffer to be read by
	 * subscribers later.
	 * @param filp
	 *   the subscriber; this is not used.
	 * @param buffer
	 *   The buffer for the input data
	 * @param buflen
	 *   the length of the buffer.
	 * @return size_t
	 *   The number of bytes that are written
	 */
	virtual size_t   write(Handle_t *pHandle, const char *pcBuffer, size_t xBufLen);

	/**
	 * IOCTL control for the subscriber.
	 */
	virtual int   ioctl(Handle_t *pHandle, int iCmd, void *pvArg);

	/**
	 * Method to publish a data to this node.
	 */
	static size_t publish
	(
		const orb_metadata *meta,
		orb_advert_t handle,
		const void *data
	);

	/**
	 * processes a request for add subscription from remote
	 * @param rateInHz
	 *   Specifies the desired rate for the message.
	 * @return
	 *   0 = success
	 *   otherwise failure.
	 */
	int16_t process_add_subscription(int32_t rateInHz);

	/**
	 * processes a request to remove a subscription from remote.
	 */
	int16_t process_remove_subscription();

	/**
	 * processed the received data message from remote.
	 */
	int16_t process_received_message(int32_t length, uint8_t *data);

	/**
	  * Add the subscriber to the node's list of subscriber.  If there is
	  * remote proxy to which this subscription needs to be sent, it will
	  * done via uORBCommunicator::IChannel interface.
	  * @param sd
	  *   the subscriber to be added.
	  */
	void add_internal_subscriber();

	/**
	 * Removes the subscriber from the list.  Also notifies the remote
	 * if there a uORBCommunicator::IChannel instance.
	 * @param sd
	 *   the Subscriber to be removed.
	 */
	void remove_internal_subscriber();

	/**
	 * Return true if this topic has been published.
	 *
	 * This is used in the case of multi_pub/sub to check if it's valid to advertise
	 * and publish to this node or if another node should be tried. */
	bool is_published();

protected:
	virtual pollevent_t poll_state(Handle_t *pHandle);
	virtual void poll_notify_one(struct pollfd *fds, pollevent_t events);

private:
	struct SubscriberData {
		unsigned  generation; /**< last generation the subscriber has seen */
		unsigned  update_interval; /**< if nonzero minimum interval between updates */
		struct hrt_call update_call;  /**< deferred wakeup call if update_period is nonzero */
		void    *poll_priv; /**< saved copy of fds->f_priv while poll is active */
		bool    update_reported; /**< true if we have reported the update via poll/check */
		int   priority; /**< priority of publisher */
	};

	const struct orb_metadata *_meta; /**< object metadata information */
	uint8_t     *_data;   /**< allocated object buffer */
	hrt_abstime   _last_update; /**< time the object was last updated */
	volatile unsigned   _generation;  /**< object generation count */
	pid_t     _publisher; /**< if nonzero, current publisher */
	const int   _priority;  /**< priority of topic */
	bool _published;  /**< has ever data been published */

private: // private class methods.

	SubscriberData    *handle_to_sd(Handle_t *pHandle)
	{
		SubscriberData *sd = (SubscriberData *)(pHandle->pvDriverPrivate);
		return sd;
	}

	bool    _IsRemoteSubscriberPresent;
	int32_t _subscriber_count;

	/**
	 * Perform a deferred update for a rate-limited subscriber.
	 */
	void      update_deferred();

	/**
	 * Bridge from hrt_call to update_deferred
	 *
	 * void *arg    ORBDevNode pointer for which the deferred update is performed.
	 */
	static void   update_deferred_trampoline(void *arg);

	/**
	 * Check whether a topic appears updated to a subscriber.
	 *
	 * @param sd    The subscriber for whom to check.
	 * @return    True if the topic should appear updated to the subscriber
	 */
	bool      appears_updated(SubscriberData *sd);

	// disable copy and assignment operators
	DeviceNode(const DeviceNode &);
	DeviceNode &operator=(const DeviceNode &);
};

/**
 * Master control device for ObjDev.
 *
 * Used primarily to create new objects via the ORBIOCCREATE
 * ioctl.
 */
class uORB::DeviceMaster : public device::CDev
{
public:
	DeviceMaster(Flavor f);
	virtual ~DeviceMaster();

	static uORB::DeviceNode *GetDeviceNode(const char *node_name);
	virtual int   ioctl(Handle_t *pHandle, int iCmd, void *pvArg);
private:
	Flavor      _flavor;
	static ORBMap _node_map;
};



#endif
