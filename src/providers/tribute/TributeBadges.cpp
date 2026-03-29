#include "providers/tribute/TributeBadges.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "messages/Image.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSize>
#include <QUrl>

namespace chatterino::tribute {

namespace {

    const QString TRIBUTE_API_BASE = "https://tributealerts.nekittop4ik.space";

    QString resolveUrl(const QString &url)
    {
        if (url.startsWith('/'))
        {
            return TRIBUTE_API_BASE + url;
        }
        return url;
    }

    std::optional<TributeBadge> parseBadge(const QJsonObject &json, const QString &id)
    {
        if (json.isEmpty())
        {
            return std::nullopt;
        }

        auto url = resolveUrl(json.value("url").toString());
        auto title = json.value("title").toString();

        if (url.isEmpty())
        {
            return std::nullopt;
        }

        auto emote = Emote{
            .name = EmoteName{u"tribute:" % id},
            .images = ImageSet{Image::fromUrl(Url{url}, 0.0, QSize(18, 18))},
            .tooltip = Tooltip{title},
            .homePage = Url{},
            .id = EmoteId{id},
        };

        return TributeBadge{std::make_shared<const Emote>(std::move(emote))};
    }

} // namespace

void TributeBadges::loadChannel(
    std::weak_ptr<Channel> channel, const QString &channelName,
    std::function<void(TributeChannelData &&)> successCallback,
    std::function<void()> failureCallback)
{
    QString url = TRIBUTE_API_BASE + "/api/v1/badges/" + channelName + "/all";

    NetworkRequest(url)
        .timeout(20000)
        .onSuccess([successCallback = std::move(successCallback),
                    failureCallback](const auto &result) {
            const auto json = result.parseJson();

            if (!json.value("success").toBool())
            {
                if (failureCallback) failureCallback();
                return;
            }

            TributeChannelData data;

            // API v1: channel_badge
            if (json.contains("channel_badge") && !json.value("channel_badge").isNull())
            {
                data.channelBadge = parseBadge(json.value("channel_badge").toObject(), "channel_badge");
            }

            // API v2: channel_badge_tiers
            if (json.contains("channel_badge_tiers"))
            {
                auto tiersObj = json.value("channel_badge_tiers").toObject();
                for (auto it = tiersObj.begin(); it != tiersObj.end(); ++it)
                {
                    auto tierIdStr = it.key();
                    auto tierBadge = parseBadge(it.value().toObject(), "tier_" + tierIdStr);
                    if (tierBadge)
                    {
                        data.channelBadgeTiers[tierIdStr.toInt()] = *tierBadge;
                    }
                }
            }

            // service_badges
            if (json.contains("service_badges"))
            {
                auto serviceObj = json.value("service_badges").toObject();
                for (auto it = serviceObj.begin(); it != serviceObj.end(); ++it)
                {
                    auto badgeId = it.key();
                    auto serviceBadge = parseBadge(it.value().toObject(), "svc_" + badgeId);
                    if (serviceBadge)
                    {
                        data.serviceBadges[badgeId] = *serviceBadge;
                    }
                }
            }

            // users
            if (json.contains("users"))
            {
                auto usersObj = json.value("users").toObject();
                for (auto it = usersObj.begin(); it != usersObj.end(); ++it)
                {
                    auto username = it.key().toLower();
                    auto userObj = it.value().toObject();

                    TributeUser user;
                    user.isSubscriber = userObj.value("is_subscriber").toBool();
                    
                    if (userObj.contains("channel_badge_tier_id") && !userObj.value("channel_badge_tier_id").isNull())
                    {
                        user.channelBadgeTierId = userObj.value("channel_badge_tier_id").toInt();
                    }
                    user.hasChannelBadge = userObj.value("has_channel_badge").toBool();
                    
                    auto serviceBadgeIdsArray = userObj.value("service_badge_ids").toArray();
                    for (const auto &idVal : serviceBadgeIdsArray)
                    {
                        user.serviceBadgeIds.push_back(idVal.toString());
                    }

                    data.users[username] = std::move(user);
                }
            }

            successCallback(std::move(data));
        })
        .onError([failureCallback](const auto &result) {
            if (failureCallback) failureCallback();
        })
        .execute();
}

} // namespace chatterino::tribute
