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

#include <stdarg.h>
#include "uORBUtils.hpp"
#include "uORBManager.hpp"


//=========================  Static initializations =================
uORB::Manager *uORB::Manager::_Instance = NULL;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
uORB::Manager *uORB::Manager::get_instance()
{
	if (_Instance == NULL) {
		_Instance = new uORB::Manager();
	}

	return _Instance;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
uORB::Manager::Manager()
	: _comm_channel(NULL)
{
}

int uORB::Manager::orb_exists(const struct orb_metadata *meta, int instance)
{
	/*
	 * Generate the path to the node and try to open it.
	 */
	char path[orb_maxpath];
	int inst = instance;
	int ret = uORB::Utils::node_mkpath(path, PUBSUB, meta, &inst);

	if (ret != 0) {
		return uORB::ERROR;
	}

	return DriverDevExists(path);
}

orb_advert_t uORB::Manager::orb_advertise(const struct orb_metadata *meta, const void *data)
{
	return orb_advertise_multi(meta, data, NULL, ORB_PRIO_DEFAULT);
}

orb_advert_t uORB::Manager::orb_advertise_multi(const struct orb_metadata *meta, const void *data, int *instance,
		int priority)
{
	int result;
    int fd;
	orb_advert_t advertiser;

	/* open the node as an advertiser */
	fd = node_open(PUBSUB, meta, data, true, instance, priority);

	if (fd == -1) {
        Print_Err("node open failed\n");
		return NULL;
	}

	/* get the advertiser handle and close the node */
	result = ioctl(fd, ORBIOCGADVERTISER, (void *)&advertiser);
    //Print_Info("advertiser=%x\n", (int)advertiser);
	close(fd);

	if (result == ERROR) {
        Print_Err("ioctl ORBIOCGADVERTISER failed!!\n");
		return NULL;
	}

	/* the advertiser must perform an initial publish to initialise the object */
	result = orb_publish(meta, advertiser, data);

	if (result == ERROR) {
        Print_Err("initial publish failed!!\n");
		return NULL;
	}

	return advertiser;
}

int uORB::Manager::orb_subscribe(const struct orb_metadata *meta)
{
	return node_open(PUBSUB, meta, NULL, false);
}

int uORB::Manager::orb_subscribe_multi(const struct orb_metadata *meta, unsigned instance)
{
	int inst = instance;
	return node_open(PUBSUB, meta, NULL, false, &inst);
}

int uORB::Manager::orb_unsubscribe(int handle)
{
	return close(handle);
}

int uORB::Manager::orb_publish(const struct orb_metadata *meta, orb_advert_t handle, const void *data)
{
	return uORB::DeviceNode::publish(meta, handle, data);
}

int uORB::Manager::orb_copy(const struct orb_metadata *meta, int handle, void *buffer)
{
	int ret;

	ret = read(handle, (char *)buffer, meta->o_size);

	if (ret < 0) {
	    return ERROR;
	}

	if (ret != (int)meta->o_size) {
	    return ERROR;
	}

	return 0;
}

int uORB::Manager::orb_check(int handle, bool *updated)
{
	return ioctl(handle, ORBIOCUPDATED, (void *)updated);
}

int uORB::Manager::orb_stat(int handle, uint64_t *time)
{
	return ioctl(handle, ORBIOCLASTUPDATE, (void *)time);
}

int uORB::Manager::orb_priority(int handle, int32_t *priority)
{
	return ioctl(handle, ORBIOCGPRIORITY, (void *)priority);
}

int uORB::Manager::orb_set_interval(int handle, unsigned interval)
{
    unsigned tmp_interval = interval*1000;
	return ioctl(handle, ORBIOCSETINTERVAL, (void *)&tmp_interval);
}


int uORB::Manager::node_advertise
(
	const struct orb_metadata *meta,
	int *instance,
	int priority
)
{
	int fd = -1;
	int ret = ERROR;

	/* fill advertiser data */
	const struct orb_advertdata adv = { meta, instance, priority };

	/* open the control device */
	fd = open(TOPIC_MASTER_DEVICE_PATH, 0);

	if (fd == -1) {
        Print_Err("open %s failed, uorb may not start!!\n", TOPIC_MASTER_DEVICE_PATH);
		goto out;
	}

	/* advertise the object */
	ret = ioctl(fd, ORBIOCADVERTISE, (void *)&adv);

	/* it's 0 if it already exists */

out:

	if (fd != 0) {
		close(fd);
	}

	return ret;
}

int uORB::Manager::node_open
(
	Flavor f,
	const struct orb_metadata *meta,
	const void *data,
	bool advertiser,
	int *instance,
	int priority
)
{
	char path[orb_maxpath];
	int ret;
    int fd = 0;
	/*
	 * If meta is null, the object was not defined, i.e. it is not
	 * known to the system.  We can't advertise/subscribe such a thing.
	 */
	if (NULL == meta) {
	    Print_Err("meta is null\n");	
        goto OUT;
	}

	/*
	 * Advertiser must publish an initial value.
	 */
	if (advertiser && (data == NULL)) {
	    Print_Err("Invalid argument:advertiser=%d data=0x%x\n", advertiser, (int)data);	
        goto OUT;
	}

	/*
	 * Generate the path to the node and try to open it.
	 */
	ret = uORB::Utils::node_mkpath(path, f, meta, instance);

	if (ret != 0) {
	    Print_Err("node mkpath failed\n");	
        goto OUT;
	}

	/* open the path as either the advertiser or the subscriber */
	fd = open(path, (advertiser) ? O_WRONLY : O_RDONLY);

	/* if we want to advertise and the node existed, we have to re-try again */
	if ((fd != -1) && (instance != NULL) && (advertiser)) {
      //  Print_Info("advertiser=%d\n", advertiser);
		/* close the fd, we want a new one */
		close(fd);
		/* the node_advertise call will automatically go for the next free entry */
		fd = -1;
	}

	/* we may need to advertise the node... */
	if (fd == -1) {

		/* try to create the node */
		ret = node_advertise(meta, instance, priority);

		if (ret == 0) {
			/* update the path, as it might have been updated during the node_advertise call */
			ret = uORB::Utils::node_mkpath(path, f, meta, instance);

			if (ret != 0) {
                goto OUT;
			}
		}

		/* on success, try the open again */
		if (ret == 0) {
			fd = open(path, (advertiser) ? O_WRONLY : O_RDONLY);
		}
	}

	if (fd == -1) {
	    Print_Err("node open failed!!\n");
        goto OUT;
	}

OUT:
	/* everything has been 0, we can return the handle now */
	return fd;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void uORB::Manager::set_uorb_communicator(uORBCommunicator::IChannel *channel)
{
	_comm_channel = channel;

	if (_comm_channel != NULL) {
		_comm_channel->register_handler(this);
	}
}

uORBCommunicator::IChannel *uORB::Manager::get_uorb_communicator(void)
{
	return _comm_channel;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int16_t uORB::Manager::process_add_subscription(const char *messageName,
		int32_t msgRateInHz)
{
	Print_Warn("[posix-uORB::Manager::process_add_subscription(%d)] entering Manager_process_add_subscription: name: %s",
	      __LINE__, messageName);
	int16_t rc = 0;
	_remote_subscriber_topics.insert(messageName);
	char nodepath[orb_maxpath];
	int ret = uORB::Utils::node_mkpath(nodepath, PUBSUB, messageName);

	if (ret == 0) {
		// get the node name.
		uORB::DeviceNode *node = uORB::DeviceMaster::GetDeviceNode(nodepath);

		if (node == NULL) {
			Print_Warn("[posix-uORB::Manager::process_add_subscription(%d)]DeviceNode(%s) not created yet",
			      __LINE__, messageName);

		} else {
			// node is present.
			node->process_add_subscription(msgRateInHz);
		}

	} else {
		rc = -1;
	}

	return rc;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int16_t uORB::Manager::process_remove_subscription(
	const char *messageName)
{
	Print_Warn("[posix-uORB::Manager::process_remove_subscription(%d)] Enter: name: %s",
	      __LINE__, messageName);
	int16_t rc = -1;
	_remote_subscriber_topics.erase(messageName);
	char nodepath[orb_maxpath];
	int ret = uORB::Utils::node_mkpath(nodepath, PUBSUB, messageName);

	if (ret == 0) {
		uORB::DeviceNode *node = uORB::DeviceMaster::GetDeviceNode(nodepath);

		// get the node name.
		if (node == NULL) {
			Print_Warn("[posix-uORB::Manager::process_remove_subscription(%d)]Error No existing subscriber found for message: [%s]",
			      __LINE__, messageName);

		} else {
			// node is present.
			node->process_remove_subscription();
			rc = 0;
		}
	}

	return rc;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int16_t uORB::Manager::process_received_message(const char *messageName,
		int32_t length, uint8_t *data)
{
	//Print_Warn("[uORB::Manager::process_received_message(%d)] Enter name: %s", __LINE__, messageName );

	int16_t rc = -1;
	char nodepath[orb_maxpath];
	int ret = uORB::Utils::node_mkpath(nodepath, PUBSUB, messageName);

	if (ret == 0) {
		uORB::DeviceNode *node = uORB::DeviceMaster::GetDeviceNode(nodepath);

		// get the node name.
		if (node == NULL) {
			Print_Warn("[uORB::Manager::process_received_message(%d)]Error No existing subscriber found for message: [%s] nodepath:[%s]",
			      __LINE__, messageName, nodepath);

		} else {
			// node is present.
			node->process_received_message(length, data);
			rc = 0;
		}
	}

	return rc;
}

bool uORB::Manager::is_remote_subscriber_present(const char *messageName)
{
	return _remote_subscriber_topics.find(messageName);
}
