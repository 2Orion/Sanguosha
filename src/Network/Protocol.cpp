#include "Protocol.h"

namespace Protocol {

CardList redactCardList(const CardList& cards)
{
    CardList redacted;
    redacted.reserve(cards.size());
    for (const CardData& card : cards) {
        CardData placeholder;
        placeholder.cardId = card.cardId;
        placeholder.ownerId = card.ownerId;
        redacted.push_back(placeholder);
    }
    return redacted;
}

} // namespace Protocol
