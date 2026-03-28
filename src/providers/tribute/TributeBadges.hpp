// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "messages/Emote.hpp"

#include <QString>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace chatterino {

class Channel;

namespace tribute {

struct TributeBadge {
    EmotePtr emote;
};

struct TributeUser {
    bool isSubscriber = false;
    std::optional<int> channelBadgeTierId;
    bool hasChannelBadge = false;
    std::vector<QString> serviceBadgeIds;
};

struct TributeChannelData {
    std::optional<TributeBadge> channelBadge;
    std::map<int, TributeBadge> channelBadgeTiers;
    std::map<QString, TributeBadge> serviceBadges;
    // Map of lowercase twitch usernames to user data
    std::map<QString, TributeUser> users;
};

class TributeBadges
{
public:
    // channelName must be lowercase
    static void loadChannel(
        std::weak_ptr<Channel> channel, const QString &channelName,
        std::function<void(TributeChannelData &&)> successCallback,
        std::function<void()> failureCallback);
};

}  // namespace tribute
}  // namespace chatterino
