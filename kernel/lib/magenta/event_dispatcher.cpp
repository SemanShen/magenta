// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <magenta/event_dispatcher.h>

#include <assert.h>
#include <err.h>
#include <trace.h>

#include <kernel/event.h>

#include <magenta/waiter.h>

constexpr mx_rights_t kDefaultEventRights =
    MX_RIGHT_DUPLICATE | MX_RIGHT_TRANSFER | MX_RIGHT_READ | MX_RIGHT_WRITE;

status_t EventDispatcher::Create(uint32_t options, utils::RefPtr<Dispatcher>* dispatcher,
                                 mx_rights_t* rights) {
    auto disp = new EventDispatcher(options);
    if (!disp) return ERR_NO_MEMORY;

    *rights = kDefaultEventRights;
    *dispatcher = utils::AdoptRef<Dispatcher>(disp);
    return NO_ERROR;
}

EventDispatcher::EventDispatcher(uint32_t options) {}

EventDispatcher::~EventDispatcher() {}

void EventDispatcher::Close(Handle* handle) {
    waiter_.CloseHandle(handle);
}

Waiter* EventDispatcher::BeginWait(event_t* event, Handle* handle, mx_signals_t signals) {
    return waiter_.BeginWait(event, handle, signals);
}

status_t EventDispatcher::SignalEvent() {
    waiter_.Signal(MX_SIGNAL_SIGNALED);
    return NO_ERROR;
}

status_t EventDispatcher::ResetEvent() {
    waiter_.Reset();
    return NO_ERROR;
}


status_t EventDispatcher::UserSignal(uint32_t set_mask, uint32_t clear_mask) {
    waiter_.Modify(set_mask, clear_mask);
    return NO_ERROR;
}