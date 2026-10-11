#include <abyssal/misc/Attachment.hpp>
#include <abyssutils/memory/SharedPtr.hpp>
#include <abyssutils/memory/WeakPtr.hpp>
#include "shared.hpp"

class CFooAttachment : public Abyssal::IAttachment {
  public:
    int counter = 0;
};

class CBarAttachment : public Abyssal::IAttachment {
  public:
    int counter = 0;
};

int main() {
    Abyssal::CAttachmentManager attachments;
    int                            ret = 0;
    EXPECT(attachments.has<CFooAttachment>(), false);
    EXPECT(!!attachments.get<CFooAttachment>(), false);
    EXPECT(attachments.has<CBarAttachment>(), false);
    EXPECT(!!attachments.get<CBarAttachment>(), false);

    auto foo = Abyssutils::Memory::makeShared<CFooAttachment>();
    attachments.add(foo);
    EXPECT(attachments.has<CFooAttachment>(), true);
    EXPECT(attachments.has<CBarAttachment>(), false);
    foo->counter++;
    // cant work for now
    //EXPECT(attachments.get<CFooAttachment>(), foo);
    EXPECT(attachments.get<CFooAttachment>()->counter, 1);

    attachments.add(Abyssutils::Memory::makeShared<CBarAttachment>());
    EXPECT(attachments.get<CFooAttachment>()->counter, 1);
    EXPECT(attachments.get<CBarAttachment>()->counter, 0);

    Abyssutils::Memory::CWeakPointer<CBarAttachment> bar = attachments.get<CBarAttachment>();
    EXPECT(bar.valid(), true);
    bar->counter = 5;

    // test overriding an attachment
    attachments.add(Abyssutils::Memory::makeShared<CBarAttachment>());
    Abyssutils::Memory::CWeakPointer<CBarAttachment> newBar = attachments.get<CBarAttachment>();
    EXPECT(bar == newBar, false);
    EXPECT(attachments.get<CBarAttachment>()->counter, 0);

    // should be a noop as this is a different attachment
    attachments.remove(Abyssutils::Memory::makeShared<CFooAttachment>());
    EXPECT(attachments.has<CFooAttachment>(), true);
    EXPECT(attachments.has<CBarAttachment>(), true);

    attachments.remove(foo);
    EXPECT(attachments.has<CFooAttachment>(), false);
    EXPECT(attachments.has<CBarAttachment>(), true);

    attachments.removeByType<CBarAttachment>();
    EXPECT(attachments.has<CFooAttachment>(), false);
    EXPECT(attachments.has<CBarAttachment>(), false);

    EXPECT(foo.strongRef(), 1);
    EXPECT(bar.valid(), false);
    EXPECT(newBar.valid(), false);

    return ret;
}
