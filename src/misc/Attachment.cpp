#include <abyssal/misc/Attachment.hpp>

using namespace Abyssal;
using namespace Abyssutils::Memory;
#define SP CSharedPointer

void Abyssal::CAttachmentManager::add(SP<IAttachment> attachment) {
    const IAttachment& att   = *attachment;
    attachments[typeid(att)] = attachment;
}

void Abyssal::CAttachmentManager::remove(SP<IAttachment> attachment) {
    const IAttachment& att = *attachment;
    auto               it  = attachments.find(typeid(att));
    if (it != attachments.end() && it->second == attachment)
        attachments.erase(it);
}

void Abyssal::CAttachmentManager::clear() {
    attachments.clear();
}
