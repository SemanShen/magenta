// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <stdint.h>

#include <kernel/mutex.h>

#include <magenta/dispatcher.h>
#include <magenta/msg_pipe.h>
#include <magenta/types.h>
#include <magenta/waiter.h>

#include <utils/ref_counted.h>
#include <utils/unique_ptr.h>

class MessagePipeDispatcher final : public Dispatcher {
public:
    static status_t Create(utils::RefPtr<Dispatcher>* dispatcher0,
                           utils::RefPtr<Dispatcher>* dispatcher1, mx_rights_t* rights);

    ~MessagePipeDispatcher() final;
    void Close(Handle* handle) final;
    Waiter* BeginWait(event_t* event, Handle* handle, mx_signals_t signals) final;
    MessagePipeDispatcher* get_message_pipe_dispatcher() final { return this; }

    status_t BeginRead(uint32_t* message_size, uint32_t* handle_count);
    status_t AcceptRead(utils::Array<uint8_t>* data, utils::Array<Handle*>* handles);
    status_t Write(utils::Array<uint8_t> data, utils::Array<Handle*> handles);

private:
    MessagePipeDispatcher(size_t side, utils::RefPtr<MessagePipe> pipe);

    size_t side_;
    utils::RefPtr<MessagePipe> pipe_;
    utils::unique_ptr<MessagePacket> pending_;
    mutex_t lock_;
};