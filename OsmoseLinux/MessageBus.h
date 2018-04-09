/*
 * Message.h
 *
 *  Created on: Mar 24, 2018
 *      Author: marc
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <string>
#include <functional>
#include <queue>
#include <vector>
#include "json/json.h"

namespace std {

class Message
{
public:
    Message(const std::string MsgTo, const std::string event,Json::Value root)
    {
        messageEvent = event;
        messageTo    = MsgTo;
        rootJson	 = root;
    }

    std::string getEvent()
    {
        return messageEvent;
    }
    std::string getMsgFor()
    {
           return messageTo;
    }

	const Json::Value& getRootJson() const {
		return rootJson;
	}

private:
    std::string messageEvent;
    std::string messageTo;
    Json::Value rootJson;
};

class MessageBus
{
public:
    MessageBus() {};
    ~MessageBus() {};

    void addReceiver(std::function<void (Message)> messageReceiver)
    {
        receivers.push_back(messageReceiver);
    }

    void sendMessage(Message message)
    {
        messages.push(message);
    }

    void notify()
    {
        while(!messages.empty()) {
            for (auto iter = receivers.begin(); iter != receivers.end(); iter++) {
                (*iter)(messages.front());
            }

            messages.pop();
        }
    }

private:
    std::vector<std::function<void (Message)>> receivers;
    std::queue<Message> messages;
};

class MarelabBusNode // @suppress("Class has a virtual method and non-virtual destructor")
{
public:
    MarelabBusNode(MessageBus *messageBus)
    {
        this->messageBus = messageBus;
        this->messageBus->addReceiver(this->getNotifyFunc());
    }

    virtual void update() {}
protected:
    MessageBus *messageBus;

    std::function<void (Message)> getNotifyFunc()
    {
        auto messageListener = [=](Message message) -> void {
            this->onNotify(message);
        };
        return messageListener;
    }

    void send(Message message)
    {
        messageBus->sendMessage(message);
    }

    virtual void onNotify(Message message)
    {
        // Do something here. Your choice. But you could do this.
        // std::cout << "Siopao! Siopao! Siopao! (Someone forgot to implement onNotify().)" << std::endl;
    }
};

} /* namespace std */

#endif /* MESSAGE_H_ */
