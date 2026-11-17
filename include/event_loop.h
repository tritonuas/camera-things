/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Google Inc.
 *
 * event_loop.h - Event loop based on cam's
 */
#ifndef __SIMPLE_CAM_EVENT_LOOP_H__
#define __SIMPLE_CAM_EVENT_LOOP_H__

#include <atomic>

struct event_base;

class EventLoop
{
public:
	EventLoop();
	~EventLoop();

	void exit(int code = 0);
	int exec();

	void timeout(unsigned int sec);

private:
	static EventLoop *instance_;

	static void timeoutTriggered(int fd, short event, void *arg);

	struct event_base *event_;
	std::atomic<bool> exit_;
	int exitCode_;

	void interrupt();
};

#endif /* __SIMPLE_CAM_EVENT_LOOP_H__ */
