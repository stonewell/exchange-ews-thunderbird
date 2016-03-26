#include <gtest/gtest.h>
#include "libews.h"
#include <iostream>
#include <memory>

ews::CEWSString endpoint;
//const ews::CEWSString endpoint("http://");
ews::CEWSString user_name;
ews::CEWSString user_email;
ews::CEWSString password;
ews::CEWSString domain;
int auth_method;

class TestLibEWS: public testing::Test {
public:
	static void SetUpTestCase() {
	}

	static void TearDownTestCase() {
	}

	static ews::CEWSSession * m_pSession;
};

class MyEnv: public testing::Environment {
public:
	virtual ~MyEnv() {

	}

	virtual void SetUp() {
        ews::EWSConnectionInfo connInfo;
        ews::EWSAccountInfo accInfo;

        connInfo.Endpoint = endpoint;
        connInfo.AuthMethod = (ews::EWSConnectionInfo::EWSAuthMethodEnum)auth_method;

        accInfo.UserName = user_name;
        accInfo.Password = password;
        accInfo.Domain = domain;
        accInfo.EmailAddress = user_email;
        
		TestLibEWS::m_pSession = ews::CEWSSession::CreateSession(&connInfo,
                                                                 &accInfo,
                                                                 NULL,
                                                                 NULL);

        if (endpoint.IsEmpty()) {
            TestLibEWS::m_pSession->AutoDiscover(NULL);
        }
	}

	virtual void TearDown() {
		delete TestLibEWS::m_pSession;
		std::cout << "Tear Down Called" << std::endl;
	}
};

testing::AssertionResult IsTrue(bool v, const ews::CEWSString & msg) {
	if (v)
		return testing::AssertionSuccess();
	else
		return testing::AssertionFailure() << msg << std::endl;
}

ews::CEWSSession * TestLibEWS::m_pSession;

TEST_F(TestLibEWS, FolderList) {
	std::auto_ptr<ews::CEWSFolderOperation> pOperation(
        TestLibEWS::m_pSession->CreateFolderOperation());

	ews::CEWSError error;
	std::auto_ptr<ews::CEWSFolderList> pFolders(
        pOperation->List(ews::CEWSFolder::Folder_Mail, NULL, &error));

	ASSERT_TRUE(IsTrue(pFolders.get() != NULL, error.GetErrorMessage()));

	ews::CEWSFolderList::iterator it;
	for (it = pFolders->begin(); it != pFolders->end(); it++) {
		std::cout << "visible:" << (*it)->IsVisible() << ",display name:"
                  << (*it)->GetDisplayName() << ",id=" << (*it)->GetFolderId();
		if ((*it)->GetParentFolder())
			std::cout << ",parent folder:"
                      << (*it)->GetParentFolder()->GetDisplayName();
		std::cout << std::endl;
	}

    pFolders.reset(
        pOperation->List(ews::CEWSFolder::Folder_Mail, ews::CEWSFolder::root, &error));

	ASSERT_TRUE(IsTrue(pFolders.get() != NULL, error.GetErrorMessage()));

	for (it = pFolders->begin(); it != pFolders->end(); it++) {
		std::cout << "visible:" << (*it)->IsVisible() << ",display name:"
                  << (*it)->GetDisplayName() << ",id=" << (*it)->GetFolderId();
		if ((*it)->GetParentFolder())
			std::cout << ",parent folder:"
                      << (*it)->GetParentFolder()->GetDisplayName();
		std::cout << std::endl;
	}
}

TEST_F(TestLibEWS, FolderOP) {
	std::auto_ptr<ews::CEWSFolderOperation> pOperation(
        TestLibEWS::m_pSession->CreateFolderOperation());

	ews::CEWSError error;

	std::cout << "...............Create Folder" << std::endl;

	std::auto_ptr<ews::CEWSFolder> pFolder(
        ews::CEWSFolder::CreateInstance(ews::CEWSFolder::Folder_Mail,
                                        "test_folder"));

	ASSERT_TRUE(
        IsTrue(pOperation->Create(pFolder.get(), NULL, &error), error.GetErrorMessage()));

	pFolder->SetDisplayName("test_folder_updated");

	sleep(10);
	std::cout << "...............Update Folder" << std::endl;
	ASSERT_TRUE(
        IsTrue(pOperation->Update(pFolder.get(), &error),
               error.GetErrorMessage()));

	sleep(20);
	std::cout << "...............Delete Folder" << std::endl;
	ASSERT_TRUE(
	    IsTrue(pOperation->Delete(pFolder.get(), false, &error),
               error.GetErrorMessage()));
}

TEST_F(TestLibEWS, FolderOP2) {
	std::auto_ptr<ews::CEWSFolderOperation> pOperation(
        TestLibEWS::m_pSession->CreateFolderOperation());

	ews::CEWSError error;
	int folder_id_names[] = {
		ews::CEWSFolder::inbox,
		ews::CEWSFolder::msgfolderroot,
		ews::CEWSFolder::junkemail,
        EWSDistinguishedFolderIdName_calendar,
        EWSDistinguishedFolderIdName_contacts,
        EWSDistinguishedFolderIdName_deleteditems,
        EWSDistinguishedFolderIdName_drafts,
        EWSDistinguishedFolderIdName_inbox,
        EWSDistinguishedFolderIdName_journal,
        EWSDistinguishedFolderIdName_notes,
        EWSDistinguishedFolderIdName_outbox,
        EWSDistinguishedFolderIdName_sentitems,
        EWSDistinguishedFolderIdName_tasks,
        EWSDistinguishedFolderIdName_msgfolderroot,
        EWSDistinguishedFolderIdName_publicfoldersroot,
        EWSDistinguishedFolderIdName_root,
        EWSDistinguishedFolderIdName_junkemail,
        EWSDistinguishedFolderIdName_searchfolders,
        EWSDistinguishedFolderIdName_voicemail
	};
	
	std::auto_ptr<ews::CEWSFolderList> pFolders(
	    pOperation->GetFolders(folder_id_names,
	                           sizeof(folder_id_names) / sizeof(int),
	                           &error));

	std::cout << "error code:" << error.GetErrorCode() << std::endl;
	
	ASSERT_TRUE(IsTrue(pFolders.get() != NULL, error.GetErrorMessage()));

	ews::CEWSFolderList::iterator it;
	for (it = pFolders->begin(); it != pFolders->end(); it++) {
		if (!(*it))
			continue;
		std::cout << "visible:" << (*it)->IsVisible() << ",display name:"
                  << (*it)->GetDisplayName() << ",id=" << (*it)->GetFolderId();
		if ((*it)->GetParentFolder())
			std::cout << ",parent folder:"
                      << (*it)->GetParentFolder()->GetDisplayName();
		std::cout << std::endl;
	}

	ews_folder * folder = NULL;
	const char * folder_id = (*pFolders->begin())->GetFolderId();
	char * err_msg = NULL;
	
    int ret =
			ews_get_folders_by_id((ews_session *)TestLibEWS::m_pSession,
			                      &folder_id, 1,
			                      &folder,
                                  &err_msg);

    std::cout << "ret:" << ret
              << ",msg:" << (err_msg ? err_msg : "")
              << ","
              << folder->display_name << ","
              << folder->id << ","
              << folder->change_key << std::endl;

    ews_free_folders(folder, 1);
    if (err_msg) delete err_msg;

    //int folder_name = EWSDistinguishedFolderIdName_sentitems;

    ret =
            ews_get_folders_by_distinguished_id_name((ews_session *)TestLibEWS::m_pSession,
                                                     folder_id_names,
                                                     sizeof(folder_id_names) / sizeof(int),
                                                     &folder,
                                                     &err_msg);

    for(unsigned int i=0;i<sizeof(folder_id_names) / sizeof(int);i++) {
        if (!folder[i].id)
            continue;
        std::cout << "ret:" << ret
                  << ",msg:" << (err_msg ? err_msg : "")
                  << ","
                  << folder[i].display_name << ","
                  << folder[i].id << ","
                  << folder[i].change_key << std::endl;
    }
    ews_free_folders(folder,sizeof(folder_id_names) / sizeof(int));
    if (err_msg) delete err_msg;
}

class TestItemCallback: public ews::CEWSItemOperationCallback {
public:
	TestItemCallback() :
        m_SyncState("")
        , MaxReturnItemCount(1)
        , includeMimeContent(ews::CEWSItemOperation::MimeContent | ews::CEWSItemOperation::MessageItem) {

	}
	virtual ~TestItemCallback() {

	}
public:
	ews::CEWSStringList m_IgnoreList;
	ews::CEWSString m_SyncState;
    int MaxReturnItemCount;
    int includeMimeContent;
    
	virtual const ews::CEWSString & GetSyncState() const {
		return m_SyncState;
	}
	virtual int GetSyncMaxReturnItemCount() const {
		return MaxReturnItemCount;
	}

	virtual const ews::CEWSStringList & GetIgnoreItemIdList() const {
		return m_IgnoreList;
	}

	virtual void SetSyncState(const ews::CEWSString & syncState) {
		std::cout << "Sync State:" << syncState << std::endl;
		m_SyncState = syncState;
	}

	void OutputRecipient(const ews::CEWSString & tag,
                         const ews::CEWSRecipient * pRecipient) {
		if (!pRecipient)
			return;

		std::cout << tag << ":" << pRecipient->GetName() << ","
                  << pRecipient->GetEmailAddress() << pRecipient->GetItemId()
                  << std::endl;
	}

	void OutputRecipientList(const ews::CEWSString & tag,
                             const ews::CEWSRecipientList * pRecipientList) {
		ews::CEWSRecipientList::const_iterator it;
		int count = 0;

		std::cout << tag << std::endl;
		for (it = pRecipientList->begin(); it != pRecipientList->end(); it++) {
			char buf[255];
			sprintf(buf, "%d", count++);
			OutputRecipient(buf, *it);
		}
	}

	void OutputMessageItem(const ews::CEWSMessageItem * pItem) {
		OutputRecipient("Sender", pItem->GetSender());
		OutputRecipient("From", pItem->GetFrom());
		OutputRecipient("ReceivedBy", pItem->GetReceivedBy());
		OutputRecipient("ReceivedRepresenting",
                        pItem->GetReceivedRepresenting());

		OutputRecipientList("To", pItem->GetToRecipients());
		OutputRecipientList("Cc", pItem->GetCcRecipients());
		OutputRecipientList("Bcc", pItem->GetBccRecipients());
		OutputRecipientList("ReplyTo", pItem->GetReplyTo());

		std::cout << "Internet MessageId:" << pItem->GetInternetMessageId()
                  << std::endl;

		OutputAttachment(pItem->GetAttachments());
	}

	void OutputAttachment(const ews::CEWSAttachmentList * pAttachmentList) {
		ews::CEWSAttachmentList::const_iterator it;

		for (it = pAttachmentList->begin(); it != pAttachmentList->end();
             it++) {
			std::cout << "AttachmentId:" << (*it)->GetAttachmentId()
                      << "Content ID:" << (*it)->GetContentId()
                      << "Content Location:" << (*it)->GetContentLocation()
                      << "Content Type:" << (*it)->GetContentType() << "Name:"
                      << (*it)->GetName() << std::endl;
		}
	}

	void OutputItem(const ews::CEWSItem * pItem) {
		std::cout << "Body:" << pItem->GetBodyType() << ",[" << pItem->GetBody()
                  << "]" << std::endl;
		std::cout << "MimeContent:[" << std::endl <<  pItem->GetMimeContent()
                  << std::endl << "]" << std::endl;
		std::cout << "Subject:" << pItem->GetSubject() << std::endl;
		std::cout << "DisplayCc:" << pItem->GetDisplayCc() << std::endl;
		std::cout << "DisplayTo:" << pItem->GetDisplayTo() << std::endl;
		std::cout << "InReplyTo" << pItem->GetInReplyTo() << std::endl;
		std::cout << "ItemId:" << pItem->GetItemId() << std::endl;
		std::cout << "CreateTime:" << pItem->GetCreateTime() << std::endl;
		std::cout << "ReceivedTime:" << pItem->GetReceivedTime() << std::endl;
		std::cout << "SentTime:" << pItem->GetSentTime() << std::endl;

		if (pItem->GetItemType() == ews::CEWSItem::Item_Message) {
			OutputMessageItem(
			    dynamic_cast<const ews::CEWSMessageItem *>(pItem));
		}
	}
    
	virtual void NewItem(const ews::CEWSItem * pItem) {
		std::auto_ptr<ews::CEWSItemOperation> pOperation(
            TestLibEWS::m_pSession->CreateItemOperation());

		std::cout << "New Item:" << pItem->GetItemId()
                  << "," << pItem->GetItemType()
                  << std::endl;

        if (MaxReturnItemCount == 1) {
            ews::CEWSError error;
            std::auto_ptr<ews::CEWSItem> pFullItem(
                pOperation->GetItem(pItem->GetItemId(), includeMimeContent, &error));

            if (pFullItem.get() != NULL)
                OutputItem(pFullItem.get());
            else
                std::cout << error.GetErrorMessage() << std::endl;

            ews_item * item = NULL;
            char * err = NULL;
            ews_get_item((ews_session*)TestLibEWS::m_pSession,
                         pItem->GetItemId().GetData(),
                         includeMimeContent,
                         &item,
                         &err);

            if (item) {
                if (item->item_type == EWS_Item_Calendar) {
                    ews_calendar_item * cItem =
                            (ews_calendar_item *)item;

                    std::cout << cItem->location
                              << "," << cItem->when
                              << "," << cItem->organizer->mailbox.email
                              << std::endl;
                }
            } else {
                std::cout << "Unable to get Item:"
                          << err
                          << std::endl;
            }

            ews_free_item(item);
            if (err) free(err);
        }
	}

	virtual void UpdateItem(const ews::CEWSItem * pItem) {
		std::cout << "Update Item" << std::endl;
		OutputItem(pItem);
	}

	virtual void DeleteItem(const ews::CEWSItem * pItem) {
		std::cout << "Delete Item" << std::endl;
		OutputItem(pItem);
	}

	virtual void ReadItem(const ews::CEWSItem * pItem, bool bRead) {
		std::cout << "Read Item, Read=" << bRead << std::endl;
		OutputItem(pItem);
	}
};

ews::CEWSFolder * GetFolderByName(const ews::CEWSString & name) {
	std::auto_ptr<ews::CEWSFolderOperation> pOperation(
        TestLibEWS::m_pSession->CreateFolderOperation());

	ews::CEWSError error;
	std::auto_ptr<ews::CEWSFolderList> pFolders(
        pOperation->List(ews::CEWSFolder::Folder_Mail, NULL, &error));

	if (pFolders.get() == NULL) {
		std::cout << "Error:" << error.GetErrorMessage() << std::endl;
		return NULL;
	}

	ews::CEWSFolderList::iterator it;
	for (it = pFolders->begin(); it != pFolders->end(); it++) {
		if (!name.CompareTo((*it)->GetDisplayName())) {
			ews::CEWSFolder * pFolder = *it;

			pFolders->erase(it);

			return pFolder;
		}
	}

	return NULL;
}

TEST_F(TestLibEWS, SyncItem) {
	std::auto_ptr<ews::CEWSItemOperation> pOperation(
        TestLibEWS::m_pSession->CreateItemOperation());

	ews::CEWSError error;
	TestItemCallback callback;
	std::auto_ptr<ews::CEWSFolder> pFolder(GetFolderByName("Inbox"));
	ASSERT_TRUE(IsTrue(pFolder.get() != NULL, "Scanned Image not found"));

	ASSERT_TRUE(
        IsTrue(pOperation->SyncItems(pFolder.get(), &callback, &error),
               error.GetErrorMessage()));
	ASSERT_TRUE(
        IsTrue(pOperation->SyncItems(pFolder.get(), &callback, &error),
               error.GetErrorMessage()));
}

TEST_F(TestLibEWS, SyncCalendarItems) {
	std::auto_ptr<ews::CEWSCalendarOperation> pOperation(
        TestLibEWS::m_pSession->CreateCalendarOperation());

	ews::CEWSError error;
	TestItemCallback callback;
    callback.MaxReturnItemCount = 1;
    callback.includeMimeContent = ews::CEWSItemOperation::None;
	ASSERT_TRUE(
        IsTrue(pOperation->SyncCalendar(&callback, &error),
               error.GetErrorMessage()));
}

TEST_F(TestLibEWS, SyncCalendarItems_2) {
	std::auto_ptr<ews::CEWSCalendarOperation> pOperation(
        TestLibEWS::m_pSession->CreateCalendarOperation());

	ews::CEWSError error;
	TestItemCallback callback;
    callback.MaxReturnItemCount = 1;
    callback.includeMimeContent = ews::CEWSItemOperation::MimeContent | ews::CEWSItemOperation::CalendarItem;
	ASSERT_TRUE(
        IsTrue(pOperation->SyncCalendar(&callback, &error),
               error.GetErrorMessage()));
}

TEST_F(TestLibEWS, SendItem) {
	std::auto_ptr<ews::CEWSItemOperation> pOperation(
        TestLibEWS::m_pSession->CreateItemOperation());

	ews::CEWSError error;

	std::auto_ptr<ews::CEWSMessageItem> pItem(
        ews::CEWSMessageItem::CreateInstance());

	ews::CEWSRecipientList to;
	ews::CEWSRecipientList cc;
	ews::CEWSRecipientList bcc;

	std::auto_ptr<ews::CEWSRecipient> pTo(ews::CEWSRecipient::CreateInstance());
	pTo->SetEmailAddress("stone_si@symantec.com");
	to.push_back(pTo.release());

	std::auto_ptr<ews::CEWSRecipient> pCc(ews::CEWSRecipient::CreateInstance());
	pCc->SetEmailAddress("jingnan.si@gmail.com");
	cc.push_back(pCc.release());

	std::auto_ptr<ews::CEWSRecipient> pBcc(
        ews::CEWSRecipient::CreateInstance());
	pBcc->SetEmailAddress("stone_well@hotmail.com");
	bcc.push_back(pBcc.release());

	pItem->SetBody("Test Send with libews");
	pItem->SetSubject("Test Send With libews");
	pItem->SetToRecipients(&to);
	pItem->SetCcRecipients(&cc);
	pItem->SetBccRecipients(&bcc);

	ews::CEWSAttachmentList attachments;
	std::auto_ptr<ews::CEWSFileAttachment> attachment(
        ews::CEWSFileAttachment::CreateInstance());
	attachment->SetContent("hello world");
	attachment->SetName("test_attachment.txt");
	attachments.push_back(attachment.release());

	pItem->SetAttachments(&attachments);

	ASSERT_TRUE(IsTrue(pOperation->SendItem(pItem.get(), &error),
                       error.GetErrorMessage()));
}

TEST_F(TestLibEWS, AutoDiscover) {
    ews_session_params params;
    char * endpoint = NULL;
    char * oab_url = NULL;
    char * err_msg = NULL;
    int auth_method;
  
    memset(&params, 0, sizeof(ews_session_params));
    params.domain = "";
    params.user = user_name.GetData();
    params.password = password.GetData();
    params.email = user_email.GetData();
    
    int retcode = ews_autodiscover(&params,
                                   &endpoint, &oab_url, &auth_method,
                                   &err_msg);

    std::cout << retcode << "," << (endpoint == NULL ? "(NULL)" : endpoint )
              << "," << (oab_url == NULL ? "(NULL)" : oab_url )
              << ", auth method=" << (auth_method == EWS_AUTH_NTLM ? "NTLM" : "BASIC")
              << "," << (err_msg == NULL ? "(NULL)" : err_msg )
              << std::endl;

    if (endpoint) free(endpoint);
    if (oab_url) free(oab_url);
    if (err_msg) free(err_msg);
}

static void new_mail(const char * parent_folder_id,
                     const char * id,
                     bool is_item, void * user_data) {
    (void)user_data;
    std::cout << "push new mail:"
              << parent_folder_id
              << ","
              << id
              << ","
              << is_item
              << std::endl;
}

TEST_F(TestLibEWS, Push) {
    ews_session_params params;
    char * err_msg = NULL;
    ews_session * session = NULL;
    int inbox_id = EWSDistinguishedFolderIdName_inbox;
    ews_event_notification_callback callback;
    ews_subscription * subscription = NULL;
    int retcode = EWS_SUCCESS;
    ews_notification_server * server = NULL;
    char * url = NULL;

    memset(&params, 0, sizeof(ews_session_params));
    params.domain = domain;
    params.user = user_name;
    params.endpoint = endpoint.GetData();
    params.password = password.GetData();
    retcode = ews_session_init(&params,
                               &session,
                               &err_msg);
  
    if (retcode != EWS_SUCCESS) {
        std::cout << "session init fail with msg:" << err_msg << std::endl;
        goto END;
    }

    server = ews_create_notification_server(session, &callback, NULL);
  
    memset(&callback, 0, sizeof(ews_event_notification_callback));
    callback.new_mail = new_mail;

    retcode = ews_notification_server_bind(server);
    if (retcode != EWS_SUCCESS) {
        std::cout << "server bind fail" << std::endl;
        goto END;
    }

    retcode = ews_notification_server_get_url(server, &url);
    if (retcode != EWS_SUCCESS) {
        std::cout << "server bind fail" << std::endl;
        goto END;
    }

    retcode = ews_subscribe_to_folders_with_push(session,
	                                             NULL,
                                                 0,
                                                 &inbox_id,
                                                 1,
	                                             EWS_NOTIFY_EVENT_TYPE_NEWMAIL | EWS_NOTIFY_EVENT_TYPE_CREATE,
                                                 30,
                                                 url,
                                                 &callback,
                                                 &subscription,
	                                             &err_msg);

    if (retcode != EWS_SUCCESS) {
        std::cout << "subscription fail with msg:" << err_msg << std::endl;
        goto END;
    }

    // retcode = ews_notification_server_run(server);
    // if (retcode != EWS_SUCCESS) {
    // 	  std::cout << "server bind fail" << std::endl;
    // 	  goto END;
    // }

    ews_unsubscribe(session,
                    subscription,
                    NULL);
END:
    if (err_msg) free(err_msg);
    if (session) ews_session_cleanup(session);
    if (subscription) ews_free_subscription(subscription);
    if (url) free(url);
    if (server)  ews_free_notification_server(server);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    char * v = getenv("ews_test_username");

    if (v && strlen(v) > 0)
        user_name = v;
    
    v = getenv("ews_test_email");

    if (v && strlen(v) > 0)
        user_email = v;
    else
        user_email = user_name;
    
    v = getenv("ews_test_authmethod");

    if (v && strlen(v) > 0)
        auth_method = atol(v);
    else
        auth_method = EWS_AUTH_NTLM;

    v = getenv("ews_test_password");

    if (v && strlen(v) > 0)
        password = v;
    
    v = getenv("ews_test_endpoint");

    if (v && strlen(v) > 0)
        endpoint = v;

    if (user_name.IsEmpty() || password.IsEmpty()) {
        std::cerr << "please set env for ews_test_username, ews_test_password, and ews_test_endpoint"
                  << std::endl;
        return 1;
    }
    
    MyEnv env;
    env.SetUp();
    int ret = RUN_ALL_TESTS();
    env.TearDown();

    return ret;
}

TEST_F(TestLibEWS, GetUnreadItems) {
	std::auto_ptr<ews::CEWSItemOperation> pOperation(
        TestLibEWS::m_pSession->CreateItemOperation());

	ews::CEWSError error;
	TestItemCallback callback;
	std::auto_ptr<ews::CEWSFolder> pFolder(GetFolderByName("Inbox"));
	ASSERT_TRUE(IsTrue(pFolder.get() != NULL, "Scanned Image not found"));

	ASSERT_TRUE(
	    IsTrue(pOperation->GetUnreadItems(pFolder->GetFolderId(), &callback, &error),
               error.GetErrorMessage()));
}

TEST_F(TestLibEWS, DeleteItems) {
	std::auto_ptr<ews::CEWSItemOperation> pOperation(
        TestLibEWS::m_pSession->CreateItemOperation());

	std::auto_ptr<ews::CEWSMessageItem> pItem(
        ews::CEWSMessageItem::CreateInstance());

	ews::CEWSRecipientList to;
	ews::CEWSRecipientList cc;
	ews::CEWSRecipientList bcc;

	std::auto_ptr<ews::CEWSRecipient> pTo(ews::CEWSRecipient::CreateInstance());
	pTo->SetEmailAddress("stone_si@symantec.com");
	to.push_back(pTo.release());

	std::auto_ptr<ews::CEWSRecipient> pCc(ews::CEWSRecipient::CreateInstance());
	pCc->SetEmailAddress("jingnan.si@gmail.com");
	cc.push_back(pCc.release());

	std::auto_ptr<ews::CEWSRecipient> pBcc(
        ews::CEWSRecipient::CreateInstance());
	pBcc->SetEmailAddress("stone_well@hotmail.com");
	bcc.push_back(pBcc.release());

	pItem->SetBody("Test Send with libews");
	pItem->SetSubject("Test Send With libews");
	pItem->SetToRecipients(&to);
	pItem->SetCcRecipients(&cc);
	pItem->SetBccRecipients(&bcc);

	ews::CEWSAttachmentList attachments;
	std::auto_ptr<ews::CEWSFileAttachment> attachment(
        ews::CEWSFileAttachment::CreateInstance());
	attachment->SetContent("hello world");
	attachment->SetName("test_attachment.txt");
	attachments.push_back(attachment.release());

	pItem->SetAttachments(&attachments);
    
	ews::CEWSError error;
	ASSERT_TRUE(
	    IsTrue(pOperation->CreateItem(pItem.get(),
                                      ews::CEWSFolder::drafts,
                                      &error),
               error.GetErrorMessage()));

	ASSERT_TRUE(
	    IsTrue(pOperation->DeleteItem(pItem.get(), false, &error),
               error.GetErrorMessage()));
}

TEST_F(TestLibEWS, GetItems) {
	std::auto_ptr<ews::CEWSItemOperation> pOperation(
        TestLibEWS::m_pSession->CreateItemOperation());

	ews::CEWSError error;

	std::auto_ptr<ews::CEWSItem> pItem(
	    pOperation->GetItem("AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InglAABKXoL+pZ/4SIBV9U/tkx8+AB6WshU9AAA=",
	                        ews::CEWSItemOperation::MimeContent | ews::CEWSItemOperation::MessageItem, &error));
	ASSERT_TRUE(
	    IsTrue(pItem.get() != NULL,
               error.GetErrorMessage()));

	TestItemCallback callback;
	callback.OutputItem(pItem.get());

    ews::CEWSItemList itemList;
    itemList.push_back(pItem.release());
    
    pOperation->MarkItemsRead(&itemList, false, &error);
}
/*
  AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InglAABKXoL+pZ/4SIBV9U/tkx8+AB6WshbuAAA=
  AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InglAABKXoL+pZ/4SIBV9U/tkx8+AB6WshWhAAA=
  AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InglAABKXoL+pZ/4SIBV9U/tkx8+AB6WshZqAAA=
*/

TEST_F(TestLibEWS, ResolveNames) {
	std::auto_ptr<ews::CEWSContactOperation> pOperation(
        TestLibEWS::m_pSession->CreateContactOperation());

	ews::CEWSError error;

	std::auto_ptr<ews::CEWSItemList> pItemList(
	    pOperation->ResolveNames("DIRECT-joe", &error));
	ASSERT_TRUE(
	    IsTrue(pItemList.get() != NULL,
               error.GetErrorMessage()));

	ews::CEWSItemList::iterator it = pItemList->begin();

	while(it != pItemList->end()) {
		ews::CEWSContactItem * pContactItem =
				dynamic_cast<ews::CEWSContactItem*>(*it);

		std::cout << "Name:" << pContactItem->GetDisplayName().GetData() << ","
                  << "Email:" << pContactItem->GetEmailAddresses()->at(0).GetData()
                  << ","
                  << "IsMailList:" << pContactItem->IsMailList()
                  << std::endl
                  << "Name:" << pContactItem->GetName().GetData()
                  << ",EmailAddress:" << pContactItem->GetEmailAddress().GetData()
                  << ",RoutingType:" << pContactItem->GetRoutingType().GetData()
                  << ",MailboxType:" << pContactItem->GetMailboxType()
                  << std::endl;

        it++;
	}

}

TEST_F(TestLibEWS, ResolveNames_2) {
    char * err_msg = NULL;
    ews_session * session = (ews_session *) TestLibEWS::m_pSession;
    ews_item ** pItems = NULL;
    int item_count;

    int ret = ews_resolve_names(session,
                                "joe chen",
                                &pItems,
                                item_count,
                                &err_msg);
    if (ret != EWS_SUCCESS) {
        std::cout << "Failed:"
                  << err_msg
                  << std::endl;
    } else {
        for(int i=0;i < item_count;i++) {
	        ews_contact_item * p = (ews_contact_item*)pItems[i];
            std::cout << "[Name:" << p->display_name << ","
                      << "Email:" << (p->email_addresses ? p->email_addresses[0] : "")
                      << ","
                      << "IsMailList:" << p->is_maillist
                      << std::endl
                      << "Name:" << p->name
                      << ",EmailAddress:" << p->email_address
                      << ",RoutingType:" << p->routing_type
                      << ",MailboxType:" << p->mailbox_type
                      << "]"
                      << std::endl;
        }
    }

    if (err_msg) free(err_msg);
    ews_free_items(pItems, item_count);

    ASSERT_TRUE(ret == EWS_SUCCESS);
}

TEST_F(TestLibEWS, ResolveNames_3) {
    char * err_msg = NULL;
    ews_session * session = (ews_session *) TestLibEWS::m_pSession;
    ews_item ** pItems = NULL;
    int item_count;

    int ret = ews_resolve_names(session,
                                "joe^&*^*^chen",
                                &pItems,
                                item_count,
                                &err_msg);
    ASSERT_TRUE(ret == EWS_SUCCESS);
}

TEST_F(TestLibEWS, GetCalendarMasterItem) {
	std::auto_ptr<ews::CEWSItemOperation> pOperation(
        TestLibEWS::m_pSession->CreateItemOperation());

	ews::CEWSError error;

    const ews::CEWSString & itemId("AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InPtAABvCFhqbzC/To+XIiYnf+arABmqdBQyAAA=");
    
	std::auto_ptr<ews::CEWSItem> pItem(
	    pOperation->GetItem(itemId,
	                        ews::CEWSItemOperation::IdOnly | ews::CEWSItemOperation::CalendarItem |
                            ews::CEWSItemOperation::OccurrenceItem, &error));
	ASSERT_FALSE(
	    IsTrue(pItem.get() != NULL,
               error.GetErrorMessage()));
}

#define SAFE_OUTPUT_STR(v, p)                                           \
    if (v->p) {                                                         \
        std::cout << #v << "->" << #p << "=" << v->p << std::endl;      \
    } else {                                                            \
        std::cout << #v << "->" << #p << "=" << "NO VALUE" << std::endl; \
    }

#define SAFE_OUTPUT(v, p)                                       \
    std::cout << #v << "->" << #p << "=" << v->p << std::endl;

void out(ews_item * item) {
    SAFE_OUTPUT(item, item_type);
    SAFE_OUTPUT(item, body_type);
    SAFE_OUTPUT(item, create_time);
    SAFE_OUTPUT(item, received_time);
    SAFE_OUTPUT(item, sent_time);

    SAFE_OUTPUT_STR(item,item_id);
    SAFE_OUTPUT_STR(item,body);
    SAFE_OUTPUT_STR(item,display_cc);
    SAFE_OUTPUT_STR(item,display_to);
    SAFE_OUTPUT_STR(item,inreply_to);
    SAFE_OUTPUT_STR(item,subject);
    SAFE_OUTPUT_STR(item,change_key);
    SAFE_OUTPUT_STR(item,mime_content);

    SAFE_OUTPUT(item, attachment_count);
    SAFE_OUTPUT(item, has_attachments);
}
void out(ews_recipient * r) {
    SAFE_OUTPUT_STR((&r->mailbox),item_id);
    SAFE_OUTPUT_STR((&r->mailbox),name);
    SAFE_OUTPUT_STR((&r->mailbox),email);
    SAFE_OUTPUT_STR((&r->mailbox),routing_type);
    SAFE_OUTPUT_STR((&r->mailbox),change_key);
    SAFE_OUTPUT((&r->mailbox), mailbox_type);
}

void out(ews_attendee * r) {
    SAFE_OUTPUT(r, response_type);
    SAFE_OUTPUT(r, last_response_time);
    SAFE_OUTPUT_STR((&r->email_address),item_id);
    SAFE_OUTPUT_STR((&r->email_address),name);
    SAFE_OUTPUT_STR((&r->email_address),email);
    SAFE_OUTPUT_STR((&r->email_address),routing_type);
    SAFE_OUTPUT_STR((&r->email_address),change_key);
    SAFE_OUTPUT((&r->email_address), mailbox_type);
}

void OutputCalendarItem(ews_calendar_item * item) {
    out(&item->item);

    SAFE_OUTPUT_STR(item, uid);
    SAFE_OUTPUT(item, recurrence_id);
    SAFE_OUTPUT(item, date_time_stamp);
    SAFE_OUTPUT(item, start);
    SAFE_OUTPUT(item, end);
    SAFE_OUTPUT(item, original_start);
    SAFE_OUTPUT(item, all_day_event);
    SAFE_OUTPUT(item, legacy_free_busy_status);
    SAFE_OUTPUT_STR(item, location);
    SAFE_OUTPUT_STR(item, when);
    SAFE_OUTPUT(item, is_meeting);
    SAFE_OUTPUT(item, is_cancelled);
    SAFE_OUTPUT(item, is_recurring);
    SAFE_OUTPUT(item, meeting_request_was_sent);
    SAFE_OUTPUT(item, is_response_requested);
    SAFE_OUTPUT(item, calendar_item_type);
    SAFE_OUTPUT(item, my_response_type);
    
    if (!item->organizer) {
        std::cout << "item->organizer=NO VALUE" << std::endl;
    }

    out(item->organizer);
    
    SAFE_OUTPUT(item, required_attendees_count);

    for(int i=0; i < item->required_attendees_count; i++) {
        out(item->required_attendees[i]);
    }

    SAFE_OUTPUT(item, optional_attendees_count);
    for(int i=0; i < item->optional_attendees_count; i++) {
        out(item->optional_attendees[i]);
    }

    SAFE_OUTPUT(item, resources_count);
    SAFE_OUTPUT(item, conflicting_meetings_count);
    SAFE_OUTPUT(item, adjacent_meetings_count);
    SAFE_OUTPUT_STR(item, duration);
    SAFE_OUTPUT_STR(item, time_zone);
    SAFE_OUTPUT(item, appointment_reply_time);
    SAFE_OUTPUT(item, appointment_sequence_number);
    SAFE_OUTPUT(item, appointment_state);
    SAFE_OUTPUT(item, modified_occurrences_count);
    SAFE_OUTPUT(item, deleted_occurrences_count);
    SAFE_OUTPUT(item, conference_type);
    SAFE_OUTPUT(item, is_allow_new_time_proposal);
    SAFE_OUTPUT(item, is_online_meeting);
    SAFE_OUTPUT_STR(item, meeting_workspace_url);
    SAFE_OUTPUT_STR(item, net_show_url);
    SAFE_OUTPUT_STR(item, recurring_master_id);
}

TEST_F(TestLibEWS, GetCalendarItem) {
    const char * itemId("AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InPtAABKXoL+pZ/4SIBV9U/tkx8+AEn4x2ZBAAA=");
    char * err_msg = NULL;
    ews_calendar_item * item;
    int ret = ews_get_item((ews_session*)TestLibEWS::m_pSession,
                           itemId,
                           EWS_GetItems_Flags_MimeContent |
                           EWS_GetItems_Flags_CalendarItem,
                           (ews_item **)&item,
                           &err_msg);
                           
	ASSERT_TRUE(
	    IsTrue(ret == EWS_SUCCESS,
               err_msg ? err_msg : "NO ERR MSG"));
    
	ASSERT_TRUE(
	    IsTrue(item != NULL,
               err_msg ? err_msg : "NO ERR MSG"));

    OutputCalendarItem(item);

    if (err_msg) free(err_msg);
    
    free(item->item.item_id);
    free(item->item.change_key);

    item->item.item_id = NULL;
    item->item.change_key = NULL;

    ret =
            ews_create_item_by_distinguished_id_name((ews_session*)TestLibEWS::m_pSession,
                                                     EWSDistinguishedFolderIdName_calendar,
                                                     &item->item,
                                                     &err_msg);
                                                   
	ASSERT_TRUE(
	    IsTrue(ret == EWS_SUCCESS,
               err_msg ? err_msg : "NO ERR MSG"));

    ews_free_item(&item->item);
}

const char * encoded_mime_content =
                           "QkVHSU46VkNBTEVOREFSDQpNRVRIT0Q6UFVCTElTSA0KUFJPRElEOi0vL01vemlsbGEub3JnL05P"
                           "TlNHTUwgTW96aWxsYSBDYWxlbmRhciBWMS4xLy9FTg0KVkVSU0lPTjoyLjANCkJFR0lOOlZUSU1F"
                           "Wk9ORQ0KVFpJRDpBbWVyaWNhL0xvc19BbmdlbGVzDQpCRUdJTjpEQVlMSUdIVA0KVFpPRkZTRVRG"
                           "Uk9NOi0wODAwDQpUWk9GRlNFVFRPOi0wNzAwDQpUWk5BTUU6UERUDQpEVFNUQVJUOjE5NzAwMzA4"
                           "VDAyMDAwMA0KUlJVTEU6RlJFUT1ZRUFSTFk7QllEQVk9MlNVO0JZTU9OVEg9Mw0KRU5EOkRBWUxJ"
                           "R0hUDQpCRUdJTjpTVEFOREFSRA0KVFpPRkZTRVRGUk9NOi0wNzAwDQpUWk9GRlNFVFRPOi0wODAw"
                           "DQpUWk5BTUU6UFNUDQpEVFNUQVJUOjE5NzAxMTAxVDAyMDAwMA0KUlJVTEU6RlJFUT1ZRUFSTFk7"
                           "QllEQVk9MVNVO0JZTU9OVEg9MTENCkVORDpTVEFOREFSRA0KRU5EOlZUSU1FWk9ORQ0KQkVHSU46"
                           "VkVWRU5UDQpDUkVBVEVEOjIwMTUwOTE4VDIzNDk1MFoNCkxBU1QtTU9ESUZJRUQ6MjAxNTA5MThU"
                           "MjM1MDA4Wg0KRFRTVEFNUDoyMDE1MDkxOFQyMzUwMDhaDQpTVU1NQVJZOk5ldyBFdmVudA0KT1JH"
                           "QU5JWkVSO1JTVlA9VFJVRTtQQVJUU1RBVD1BQ0NFUFRFRDtST0xFPUNIQUlSOm1haWx0bzpzdG9u"
                           "ZV9zaUBzeW1hbnRlYy4NCiBjb20NCkFUVEVOREVFO1JTVlA9VFJVRTtQQVJUU1RBVD1ORUVEUy1B"
                           "Q1RJT047Uk9MRT1SRVEtUEFSVElDSVBBTlQ6bWFpbHRvOmppbmduDQogYW4uc2lAZ21haWwuY29t"
                           "DQpEVFNUQVJUO1RaSUQ9QW1lcmljYS9Mb3NfQW5nZWxlczoyMDE1MDkxOFQxNjQ1MDANCkRURU5E"
                           "O1RaSUQ9QW1lcmljYS9Mb3NfQW5nZWxlczoyMDE1MDkxOFQxNzQ1MDANClRSQU5TUDpPUEFRVUUN"
                           "CkxPQ0FUSU9OOlRCRA0KREVTQ1JJUFRJT046SGFoYQ0KWC1NT1otU0VORC1JTlZJVEFUSU9OUzpU"
                           "UlVFDQpYLU1PWi1TRU5ELUlOVklUQVRJT05TLVVORElTQ0xPU0VEOkZBTFNFDQpVSUQ6ZjM2Y2Uy"
                           "ZTUtZTEwYi00MzA5LTk0ZWUtYjI4NjRhMzJhMGZkDQpFTkQ6VkVWRU5UDQpFTkQ6VkNBTEVOREFS"
                           ;

const char * encoded_mime_content_2 =
                           "QkVHSU46VkNBTEVOREFSDQpNRVRIT0Q6UFVCTElTSA0KUFJPRElEOi0vL01vemlsbGEub3JnL05P"
                           "TlNHTUwgTW96aWxsYSBDYWxlbmRhciBWMS4xLy9FTg0KVkVSU0lPTjoyLjANCkJFR0lOOlZUSU1F"
                           "Wk9ORQ0KVFpJRDpQYWNpZmljIFN0YW5kYXJkIFRpbWUNCkJFR0lOOkRBWUxJR0hUDQpUWk9GRlNF"
                           "VEZST006LTA4MDANClRaT0ZGU0VUVE86LTA3MDANCkRUU1RBUlQ6MTYwMTAxMDFUMDIwMDAwDQpS"
                           "UlVMRTpGUkVRPVlFQVJMWTtCWURBWT0yU1U7QllNT05USD0zDQpFTkQ6REFZTElHSFQNCkJFR0lO"
                           "OlNUQU5EQVJEDQpUWk9GRlNFVEZST006LTA3MDANClRaT0ZGU0VUVE86LTA4MDANCkRUU1RBUlQ6"
                           "MTYwMTAxMDFUMDIwMDAwDQpSUlVMRTpGUkVRPVlFQVJMWTtCWURBWT0xU1U7QllNT05USD0xMQ0K"
                           "RU5EOlNUQU5EQVJEDQpFTkQ6VlRJTUVaT05FDQpCRUdJTjpWRVZFTlQNCkRUU1RBTVA6MjAxNTA3"
                           "MjhUMTgxMzM3Wg0KVUlEOjliNjg4ZTMyLWE4MWMtNDI1Ni1iYWFiLTIyMGMwMGQzNDIwMA0KU1VN"
                           "TUFSWTpUZWFtIENhcnJlcmEgMTUuMyBTcHJpbnQgUGxhbm5pbmcNClBSSU9SSVRZOjUNClNUQVRV"
                           "UzpDT05GSVJNRUQNClJFQ1VSUkVOQ0UtSUQ7VFpJRD1QYWNpZmljIFN0YW5kYXJkIFRpbWU6MjAx"
                           "NTA3MjlUMTAwMDAwDQpPUkdBTklaRVI7Q049SmVhbi1DbGF1ZGUgQm91cnNpcXVvdDptYWlsdG86"
                           "SmVhbi1DbGF1ZGVfQm91cnNpcXVvdEBzeW1hbnRlYy5jb20NCkRUU1RBUlQ7VFpJRD1QYWNpZmlj"
                           "IFN0YW5kYXJkIFRpbWU6MjAxNTA3MjlUMTAwMDAwDQpEVEVORDtUWklEPVBhY2lmaWMgU3RhbmRh"
                           "cmQgVGltZToyMDE1MDcyOVQxMzMwMDANCkRFU0NSSVBUSU9OO0xBTkdVQUdFPWVuLVVTOkpvaW4g"
                           "V2ViRXggbWVldGluZw0KQ0xBU1M6UFVCTElDDQpUUkFOU1A6T1BBUVVFDQpTRVFVRU5DRTo4DQpM"
                           "T0NBVElPTjtMQU5HVUFHRT1lbi1VUzpDUi5DVUwxLjAyLlBhY29pbWEgQTIwMDQuMTYNClgtTUlD"
                           "Uk9TT0ZULUNETy1BUFBULVNFUVVFTkNFOjgNClgtTUlDUk9TT0ZULUNETy1PV05FUkFQUFRJRDot"
                           "MzIwMzEzMzc3DQpYLU1JQ1JPU09GVC1DRE8tQlVTWVNUQVRVUzpURU5UQVRJVkUNClgtTUlDUk9T"
                           "T0ZULUNETy1JTlRFTkRFRFNUQVRVUzpCVVNZDQpYLU1JQ1JPU09GVC1DRE8tQUxMREFZRVZFTlQ6"
                           "RkFMU0UNClgtTUlDUk9TT0ZULUNETy1JTVBPUlRBTkNFOjENClgtTUlDUk9TT0ZULUNETy1JTlNU"
                           "VFlQRTozDQpYLU1PWi1SRUNFSVZFRC1TRVFVRU5DRTo4DQpYLU1PWi1SRUNFSVZFRC1EVFNUQU1Q"
                           "OjIwMTUwNzI4VDE2MDI1MVoNCkVORDpWRVZFTlQNCkVORDpWQ0FMRU5EQVI=";

const char * ttt =
                           "QkVHSU46VkNBTEVOREFSDQpQUk9ESUQ6LS8vTW96aWxsYS5vcmcvTk9OU0dNTCBNb3ppbGxhIENh"
                           "bGVuZGFyIFYxLjEvL0VODQpWRVJTSU9OOjIuMA0KTUVUSE9EOlBVQkxJU0gNCkJFR0lOOlZUSU1F"
                           "Wk9ORQ0KVFpJRDpBbWVyaWNhL0xvc19BbmdlbGVzDQpCRUdJTjpEQVlMSUdIVA0KVFpPRkZTRVRG"
                           "Uk9NOi0wODAwDQpUWk9GRlNFVFRPOi0wNzAwDQpUWk5BTUU6UERUDQpEVFNUQVJUOjE5NzAwMzA4"
                           "VDAyMDAwMA0KUlJVTEU6RlJFUT1ZRUFSTFk7QllEQVk9MlNVO0JZTU9OVEg9Mw0KRU5EOkRBWUxJ"
                           "R0hUDQpCRUdJTjpTVEFOREFSRA0KVFpPRkZTRVRGUk9NOi0wNzAwDQpUWk9GRlNFVFRPOi0wODAw"
                           "DQpUWk5BTUU6UFNUDQpEVFNUQVJUOjE5NzAxMTAxVDAyMDAwMA0KUlJVTEU6RlJFUT1ZRUFSTFk7"
                           "QllEQVk9MVNVO0JZTU9OVEg9MTENCkVORDpTVEFOREFSRA0KRU5EOlZUSU1FWk9ORQ0KQkVHSU46"
                           "VkVWRU5UDQpMQVNULU1PRElGSUVEOjIwMTUxMDA5VDE2NTYyOFoNCkRUU1RBTVA6MjAxNTEwMDlU"
                           "MTY1NjI4Wg0KVUlEOjA0MDAwMDAwODIwMEUwMDA3NEM1QjcxMDFBODJFMDA4MDAwMDAwMDBFMDcx"
                           "MzgxMUQwMDJEMTAxMDAwMDAwMDAwMDAwMDANCiAwMDEwMDAwMDAwRUIzRUJERUJBQ0ExNDM0Qzkw"
                           "QzRBMzdGOEY2QUMyMUUNClNVTU1BUlk6SURGcyB0byBiZSByZXZpZXdlZCBieSB0aGUgU3ltYW50"
                           "ZWMgSW5mb3JtYXRpb24gUHJvdGVjdGlvbiBQRkMgTWVlDQogdGluZyB0byBiZSBoZWxkIG9uIFRo"
                           "dXJzZGF5XCwgT2N0b2JlciAxNXRoXCwgODozMGFtLTEwOjAwYW0gUGFjaWZpYyBUaW1lICgNCiBJ"
                           "bmRpYSBUaW1lIDk6MDBwbSB0byAxMDozMHBtKQ0KUFJJT1JJVFk6NQ0KU1RBVFVTOkNPTkZJUk1F"
                           "RA0KT1JHQU5JWkVSO0NOPVBhdGVudHM6bWFpbHRvOlBhdGVudHNAc3ltYW50ZWMuY29tDQpBVFRF"
                           "TkRFRTtSU1ZQPVRSVUU7Q049QmFycmllIFJvZHk7UEFSVFNUQVQ9TkVFRFMtQUNUSU9OO1JPTEU9"
                           "UkVRLVBBUlRJQ0lQQQ0KIE5UOm1haWx0bzpCYXJyaWVfUm9keUBzeW1hbnRlYy5jb20NCkFUVEVO"
                           "REVFO1JTVlA9VFJVRTtDTj1QYXdhbmRlZXAgT2Jlcm9pO1BBUlRTVEFUPU5FRURTLUFDVElPTjtS"
                           "T0xFPVJFUS1QQVJUDQogSUNJUEFOVDptYWlsdG86UGF3YW5kZWVwX09iZXJvaUBzeW1hbnRlYy5j"
                           "b20NCkFUVEVOREVFO1JTVlA9VFJVRTtDTj1Bc2FkIEZhcnVxdWk7UEFSVFNUQVQ9TkVFRFMtQUNU"
                           "SU9OO1JPTEU9UkVRLVBBUlRJQ0lQDQogQU5UOm1haWx0bzpBc2FkX0ZhcnVxdWlAc3ltYW50ZWMu"
                           "Y29tDQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049UGFkYW0gU2luZ2FsO1BBUlRTVEFUPU5FRURTLUFD"
                           "VElPTjtST0xFPVJFUS1QQVJUSUNJUA0KIEFOVDptYWlsdG86UGFkYW1fU2luZ2FsQHN5bWFudGVj"
                           "LmNvbQ0KQVRURU5ERUU7UlNWUD1UUlVFO0NOPUhhcmkgVmVsYWRhbmRhO1BBUlRTVEFUPU5FRURT"
                           "LUFDVElPTjtST0xFPVJFUS1QQVJUSUMNCiBJUEFOVDptYWlsdG86SGFyaV9WZWxhZGFuZGFAc3lt"
                           "YW50ZWMuY29tDQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049SnVuIE1hbztQQVJUU1RBVD1ORUVEUy1B"
                           "Q1RJT047Uk9MRT1SRVEtUEFSVElDSVBBTlQ6bQ0KIGFpbHRvOkp1bl9NYW9Ac3ltYW50ZWMuY29t"
                           "DQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049U3RvbmUgU2k7UEFSVFNUQVQ9QUNDRVBURUQ7Uk9MRT1S"
                           "RVEtUEFSVElDSVBBTlQ6bWFpbA0KIHRvOlN0b25lX1NpQHN5bWFudGVjLmNvbQ0KQVRURU5ERUU7"
                           "UlNWUD1UUlVFO0NOPUF6emVkaW5lIEJlbmFtZXVyO1BBUlRTVEFUPU5FRURTLUFDVElPTjtST0xF"
                           "PVJFUS1QQVINCiBUSUNJUEFOVDptYWlsdG86QXp6ZWRpbmVfQmVuYW1ldXJAc3ltYW50ZWMuY29t"
                           "DQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049TmF0aGFuIEV2YW5zO1BBUlRTVEFUPU5FRURTLUFDVElP"
                           "TjtST0xFPVJFUS1QQVJUSUNJUA0KIEFOVDptYWlsdG86TmF0aGFuX0V2YW5zQHN5bWFudGVjLmNv"
                           "bQ0KQVRURU5ERUU7UlNWUD1UUlVFO0NOPVl1biBTaGVuO1BBUlRTVEFUPU5FRURTLUFDVElPTjtS"
                           "T0xFPVJFUS1QQVJUSUNJUEFOVDoNCiBtYWlsdG86WXVuX1NoZW5Ac3ltYW50ZWMuY29tDQpBVFRF"
                           "TkRFRTtSU1ZQPVRSVUU7Q049S3VuYWwgQWdhcndhbDtQQVJUU1RBVD1ORUVEUy1BQ1RJT047Uk9M"
                           "RT1SRVEtUEFSVElDSQ0KIFBBTlQ6bWFpbHRvOkt1bmFsX0FnYXJ3YWxAc3ltYW50ZWMuY29tDQpB"
                           "VFRFTkRFRTtSU1ZQPVRSVUU7Q049UHJhc2FkIEJva2FyZTtQQVJUU1RBVD1ORUVEUy1BQ1RJT047"
                           "Uk9MRT1SRVEtUEFSVElDSQ0KIFBBTlQ6bWFpbHRvOlByYXNhZF9Cb2thcmVAc3ltYW50ZWMuY29t"
                           "DQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049R2FyeSBLcmFsbDtQQVJUU1RBVD1ORUVEUy1BQ1RJT047"
                           "Uk9MRT1SRVEtUEFSVElDSVBBTg0KIFQ6bWFpbHRvOkdhcnlfS3JhbGxAc3ltYW50ZWMuY29tDQpB"
                           "VFRFTkRFRTtSU1ZQPVRSVUU7Q049Tmljb2xhcyBQb3BwO1BBUlRTVEFUPU5FRURTLUFDVElPTjtS"
                           "T0xFPVJFUS1QQVJUSUNJUA0KIEFOVDptYWlsdG86Tmljb2xhc19Qb3BwQHN5bWFudGVjLmNvbQ0K"
                           "QVRURU5ERUU7UlNWUD1UUlVFO0NOPUFuZHJldyBEb3duZXk7UEFSVFNUQVQ9TkVFRFMtQUNUSU9O"
                           "O1JPTEU9UkVRLVBBUlRJQ0kNCiBQQU5UOm1haWx0bzpBbmRyZXdfRG93bmV5QHN5bWFudGVjLmNv"
                           "bQ0KQVRURU5ERUU7UlNWUD1UUlVFO0NOPU1hdHQgR2Vvcmd5O1BBUlRTVEFUPU5FRURTLUFDVElP"
                           "TjtST0xFPVJFUS1QQVJUSUNJUEENCiBOVDptYWlsdG86TWF0dF9HZW9yZ3lAc3ltYW50ZWMuY29t"
                           "DQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049TWF0dGVvIERlbGwnQW1pY287UEFSVFNUQVQ9TkVFRFMt"
                           "QUNUSU9OO1JPTEU9UkVRLVBBUg0KIFRJQ0lQQU5UOm1haWx0bzpNYXR0ZW9fRGVsbEFtaWNvQHN5"
                           "bWFudGVjLmNvbQ0KQVRURU5ERUU7UlNWUD1UUlVFO0NOPUxlbiBUb3lvc2hpYmE7UEFSVFNUQVQ9"
                           "TkVFRFMtQUNUSU9OO1JPTEU9UkVRLVBBUlRJQ0kNCiBQQU5UOm1haWx0bzpMZW5fVG95b3NoaWJh"
                           "QHN5bWFudGVjLmNvbQ0KQVRURU5ERUU7UlNWUD1UUlVFO0NOPVNhbmpheSBNb2RpO1BBUlRTVEFU"
                           "PU5FRURTLUFDVElPTjtST0xFPVJFUS1QQVJUSUNJUEENCiBOVDptYWlsdG86U2FuamF5X01vZGlA"
                           "c3ltYW50ZWMuY29tDQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049Q2hyaXMgR2F0ZXM7UEFSVFNUQVQ9"
                           "TkVFRFMtQUNUSU9OO1JPTEU9UkVRLVBBUlRJQ0lQQQ0KIE5UOm1haWx0bzpDaHJpc19HYXRlc0Bz"
                           "eW1hbnRlYy5jb20NCkFUVEVOREVFO1JTVlA9VFJVRTtDTj1LZXZpbiBSb3VuZHk7UEFSVFNUQVQ9"
                           "TkVFRFMtQUNUSU9OO1JPTEU9UkVRLVBBUlRJQ0lQDQogQU5UOm1haWx0bzpLZXZpbl9Sb3VuZHlA"
                           "c3ltYW50ZWMuY29tDQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049Tmlrb2xhb3MgVmFzaWxvZ2xvdTtQ"
                           "QVJUU1RBVD1ORUVEUy1BQ1RJT047Uk9MRT1SRVEtUA0KIEFSVElDSVBBTlQ6bWFpbHRvOk5pa29s"
                           "YW9zX1Zhc2lsb2dsb3VAc3ltYW50ZWMuY29tDQpBVFRFTkRFRTtSU1ZQPVRSVUU7Q049WWluaW5n"
                           "IFdhbmc7UEFSVFNUQVQ9TkVFRFMtQUNUSU9OO1JPTEU9UkVRLVBBUlRJQ0lQQQ0KIE5UOm1haWx0"
                           "bzpZaW5pbmdfV2FuZ0BzeW1hbnRlYy5jb20NCkFUVEVOREVFO1JTVlA9VFJVRTtDTj1OaW5nIENo"
                           "YWk7UEFSVFNUQVQ9TkVFRFMtQUNUSU9OO1JPTEU9UkVRLVBBUlRJQ0lQQU5UDQogOm1haWx0bzpO"
                           "aW5nX0NoYWlAc3ltYW50ZWMuY29tDQpEVFNUQVJUO1RaSUQ9QW1lcmljYS9Mb3NfQW5nZWxlczoy"
                           "MDE1MTAxNVQwODMwMDANCkRURU5EO1RaSUQ9QW1lcmljYS9Mb3NfQW5nZWxlczoyMDE1MTAxNVQx"
                           "MDAwMDANCkRFU0NSSVBUSU9OO0xBTkdVQUdFPWVuLVVTOkRlYXIgSW52ZW50b3JzXCwKClRoZSBQ"
                           "YXRlbnQgRmlsdGVyIENvbW1pdHRlZQ0KICAoUEZDKSB3aWxsIGJlIHJldmlld2luZyB0aGUgYmVs"
                           "b3ctbWVudGlvbmVkIElERuKAmXNcLCBvbiB3aGljaCB5b3UgYXJlIGxpDQogc3RlZCBhcyBhbiBp"
                           "bnZlbnRvci4KVGhpcyBtZWV0aW5nIHdpbGwgYmUgaGVsZCBvbiBUaHVyc2RheVwsIE9jdG9iZXIg"
                           "MTV0aA0KIFwsIDg6MzBhbS0xMDowMGFtIFBhY2lmaWMgVGltZSAoSW5kaWEgVGltZSA5OjAwcG0g"
                           "dG8gMTA6MzBwbSkuIFRoZSBtb2RlcmF0DQogb3IgZm9yIHRoaXMgbWVldGluZyBpcyBCYXJyaWUg"
                           "Um9keSAoSW4taG91c2UgUGF0ZW50IEF0dG9ybmV5KS4KClRvIGhlbHAgDQogdGhlIGNvbW1pdHRl"
                           "ZSBtYWtlIGEgZGVjaXNpb24gb24geW91ciBJREZcLCB5b3Ugd2lsbCBiZSBwcmVzZW50aW5nIHlv"
                           "dXIgSUQNCiBGIHRvIHRoZSBQYXRlbnQgRmlsdGVyIENvbW1pdHRlZS4gIFRvIHByZXBhcmUgZm9y"
                           "IHRoZSBtZWV0aW5nXCwgcGxlYXNlIHJldg0KIGlldyB0aGUgaW5mb3JtYXRpb24gbG9jYXRlZCBo"
                           "ZXJlOiAgaHR0cDovL3N5bWluZm8uZ2VzLnN5bWFudGVjLmNvbS9sZWdhbC9IDQogb3dUb1ByZXNl"
                           "bnRUb1RoZVBGQy5hc3AuCgpJbnZlbnRpb24gSWQgICAgSW52ZW50aW9uIFJlZmVyZW5jZSAgICAg"
                           "SW52ZW50DQogaW9uIFRpdGxlIEludmVudG9yKHMpCjgxNjE5MTE2ICAgICAgICAxNTI0ODggIFN5"
                           "c3RlbSBhbmQgTWV0aG9kIHRvIGRldGVjdA0KICB2dWxuZXJhYmlsaXRpZXMgYW5kIGltcHJvdmUg"
                           "U2VjdXJpdHkgUG9zdHVyZSAgICAgICAgSGFyaSBWZWxhZGFuZGFcLCBBc2FkDQogIEZhcnVxdWlc"
                           "LCBQYWRhbSBTaW5nYWwKODE2MjE4OTggICAgICAgIDE1MjU1MCAgU3lzdGVtIGFuZCBtZXRob2Qg"
                           "dG8gZGV0ZQ0KIGN0IGFjY3VyYXRlIEFuZHJvaWQgaW5mb3JtYXRpb24gbGVha3MgdGhyb3VnaCBk"
                           "eW5hbWljIGFuYWx5c2lzIEp1biBNYW9cLCBKDQogaW5nbmFuIFNpCjgxNjIyNTIzICAgICAgICAx"
                           "NTI1NjcgIERpc3RyaWJ1dGVkIG5ldHdvcmsgY2FwdHVyZSBmb3IgYXR0YWNrIA0KIGFuYWx5c2lz"
                           "IGFuZCByZW1lZGlhdGlvbiBOYXRoYW4gRXZhbnNcLCBBenplZGluZSBCZW5hbWV1clwsIFl1biBT"
                           "aGVuCjgxNjINCiAzMTY1ICAgICAgICAxNTI1NzkgIFNpbmdsZSBTaWduIE9uIGFjcm9zcyBtdWx0"
                           "aXBsZSBkZXZpY2VzIGZvciBhbnkgY2xvdWQgYQ0KIHBwbGljYXRpb24KVGhpcyBpbnZlbnRpb24g"
                           "cHJvdmlkZXMgYSBub3ZpY2UgbWV0aG9kIHRvIGFsbG93IGEgdXNlciB0byBzaWcNCiBub24gb25j"
                           "ZSBhY3Jvc3MgbXVsdGlwbGUgZGV2aWNlcyBmb3IgY2xvdWQgYXBwbGljYXRpb25zLiBUaGUgaWRl"
                           "YSBpcyB0byBtYQ0KIGludGFpbiBhIGxvZ2luIGNvb2tpZSBpbiBhbiBhcHBsaWNhdGlvbiBpbnN0"
                           "YWxsZWQgb24gcGVyc29uYWwgZGV2aWNlIGxpa2UgDQogc21hcnRwaG9uZS4gICBQcmFzYWQgQm9r"
                           "YXJlXCwgR2FyeSBLcmFsbFwsIE5pY29sYXMgUG9wcFwsIEt1bmFsIEFnYXJ3YWwKOA0KIDE2MjY4"
                           "OTEgICAgICAgIDE1MjcxMCAgRmFzdCBhbmQgUmVsaWFibGUgUGFzc3dvcmQgU3RyZW5ndGggRXZh"
                           "bHVhdGlvbiAgTWF0DQogdGVvIERlbGwnQW1pY29cLCBNYXVyaXppbyBGaWxpcHBvbmUKODE1ODA3"
                           "MDcgICAgICAgIDE1MjczNSAgRHluYW1pYyBjZXJ0aQ0KIGZpY2F0ZSBpbnN0YWxsYXRpb24gdXNl"
                           "ciBpbnRlcmZhY2UgTGVuIFRveW9zaGliYVwsIENoaXQgTWVuZyBDaGVvbmcKODE2MzINCiAwODQg"
                           "ICAgICAgIDE1Mjc2MyAgV2lsZCAiVExEIiBTU0wgQ2VydGlmaWNhdGUgLSAgVGhhdCBjYW4gYmUg"
                           "dXNlZCBvbiBtdWx0aQ0KIHBsZSBUb3AgTGV2ZWwgRG9tYWlucyBmb3IgYSBnaXZlbiBkb21haW4g"
                           "bmFtZS4gICBTYW5qYXkgTW9kaQo4MTYzMjEzOSAgICANCiAgICAgMTUyNzY4ICBBdHRhY2sgUHJl"
                           "ZGljdGlvbiBiYXNlZCBvbiBDb21tdW5pdHkgRmVhdHVyZXMgICBZaW5pbmcgV2FuZ1wsIA0KIENo"
                           "cmlzdG9waGVyIEdhdGVzXCwgS2V2aW4gQWxlamFuZHJvIFJvdW5keVwsIE5pa29sYW9zIFZhc2ls"
                           "b2dsb3UKODE2MzQxMzYNCiAgICAgICAgIDE1MjgxNiAgUGVyc29uYWxpemVkIE5vcnRvbiBTZWN1"
                           "cmVkIFNlYWwgZm9yIENvbnN1bWVycyAgSGFyaSBWZWxhZA0KIGFuZGFcLCBQYWRhbSBTaW5nYWxc"
                           "LCBOaW5nIENoYWkKCllvdSBzaG91bGQgcmVjZWl2ZSBhIG5vdGljZSBvZiB0aGVpciBkZQ0KIGNp"
                           "c2lvbiB3aXRoaW4gYSB3ZWVrIG9mIHRoZSBtZWV0aW5nIOKAkyBpZiB5b3UgZG8gbm90XCwgcGxl"
                           "YXNlIGZlZWwgZnJlZSB0DQogbyBjb250YWN0IHRoZSBwYXRlbnQgZGVwYXJ0bWVudCBhdCBwYXRl"
                           "bnRzQHN5bWFudGVjLmNvbTxtYWlsdG86cGF0ZW50c0BzeW0NCiBhbnRlYy5jb20+LiAgSWYgeW91"
                           "IGhhdmUgYW55IHF1ZXN0aW9ucyBhYm91dCBvciBkaXNhZ3JlZSB3aXRoIHRoZSBQRkMgZGVjaQ0K"
                           "IHNpb25cLCBwbGVhc2UgY29udGFjdCB0aGUgaW50ZXJuYWwgcGF0ZW50IGNvdW5zZWwgYXNzaWdu"
                           "ZWQgdG8geW91ciBJREYgKGxpDQogc3RlZCBpbiBTSUFNUyBhcyDigJxDYXNlIE1hbmFnZXLigJ0p"
                           "LgoKVGhhbmsgeW91IGZvciB5b3VyIHBhcnRpY2lwYXRpb24gDQogaW4gdGhlIFN5bWFudGVjIFBh"
                           "dGVudCBQcm9ncmFtLgoKCgpSZWdhcmRzXCwKUGF3YW5kZWVwCgoNCkNMQVNTOlBVQkxJQw0KVFJB"
                           "TlNQOk9QQVFVRQ0KU0VRVUVOQ0U6MA0KTE9DQVRJT047TEFOR1VBR0U9ZW4tVVM6V2ViRXggMi4w"
                           "IGFuZCBkaWFsIGluIGRldGFpbHMgd2lsbCBiZSBzaGFyZWQgbGF0ZXINCiANClgtTUlDUk9TT0ZU"
                           "LUNETy1BUFBULVNFUVVFTkNFOjANClgtTUlDUk9TT0ZULUNETy1PV05FUkFQUFRJRDotMTI0OTI2"
                           "OTc5Mw0KWC1NSUNST1NPRlQtQ0RPLUJVU1lTVEFUVVM6VEVOVEFUSVZFDQpYLU1JQ1JPU09GVC1D"
                           "RE8tSU5URU5ERURTVEFUVVM6QlVTWQ0KWC1NSUNST1NPRlQtQ0RPLUFMTERBWUVWRU5UOkZBTFNF"
                           "DQpYLU1JQ1JPU09GVC1DRE8tSU1QT1JUQU5DRToxDQpYLU1JQ1JPU09GVC1DRE8tSU5TVFRZUEU6"
                           "MA0KWC1NT1otUkVDRUlWRUQtU0VRVUVOQ0U6MA0KWC1NT1otUkVDRUlWRUQtRFRTVEFNUDoyMDE1"
                           "MTAwOVQxNDU4MzdaDQpFTkQ6VkVWRU5UDQpFTkQ6VkNBTEVOREFS"
                           ;
TEST_F(TestLibEWS, CreateCalendarItem) {
    ews_calendar_item item;
    ews_session * session = (ews_session *) TestLibEWS::m_pSession;
    char * err_msg = NULL;
    int ret = EWS_FAIL;

    memset(&item, 0, sizeof(ews_calendar_item));
    item.item.item_type = EWS_Item_Calendar;

    printf("%s\n", encoded_mime_content);
    item.item.mime_content = (char *)ttt;//encoded_mime_content;

    ews_attendee attendee;
    ews_attendee * attendees[1] = {&attendee};
    item.required_attendees = attendees;
    item.required_attendees_count = 1;

    memset(&attendee, 0, sizeof(ews_attendee));
    attendee.email_address.name = ("Jingnan Si");
    attendee.email_address.email = ("jingnan.si@gmail.com");

    item.start = time(NULL);
    item.end = time(NULL);
    
    ret = ews_create_item_by_distinguished_id_name(session,
                                                   EWSDistinguishedFolderIdName_calendar,
                                                   &item.item,
                                                   &err_msg);

    printf("------------------%d---------%s\n", ret, err_msg ? err_msg : "");

	ASSERT_TRUE(
	    IsTrue(ret == EWS_SUCCESS,
               err_msg ? err_msg : ""));

    if (err_msg) free(err_msg);


    if (ret == EWS_SUCCESS) {
        printf("item_id=%s\nchange_key=%s\n",
               item.item.item_id ? item.item.item_id : "NULL",
               item.item.change_key ? item.item.change_key : "NULL");

        if (item.item.item_id) free(item.item.item_id);
        if (item.item.change_key) free(item.item.change_key);
    }
}

TEST_F(TestLibEWS, CalendarItemOps) {
    ews_calendar_item item;
    ews_session * session = (ews_session *) TestLibEWS::m_pSession;
    char * err_msg = NULL;
    int ret = EWS_FAIL;

    memset(&item, 0, sizeof(ews_calendar_item));
    item.item.item_type = EWS_Item_Calendar;

    printf("%s\n", encoded_mime_content);
    item.item.mime_content = (char *)encoded_mime_content;

    ews_attendee attendee;
    ews_attendee * attendees[1] = {&attendee};
    item.required_attendees = attendees;
    item.required_attendees_count = 1;

    memset(&attendee, 0, sizeof(ews_attendee));
    attendee.email_address.name = ("Jingnan Si");
    attendee.email_address.email = ("jingnan.si@gmail.com");

    item.start = time(NULL);
    item.end = time(NULL);
    
    ret = ews_create_item_by_distinguished_id_name(session,
                                                   EWSDistinguishedFolderIdName_calendar,
                                                   &item.item,
                                                   &err_msg);

    printf("------------------%d---------%s\n", ret, err_msg ? err_msg : "");

	ASSERT_TRUE(
	    IsTrue(ret == EWS_SUCCESS,
               err_msg ? err_msg : ""));

    if (err_msg) free(err_msg);


    printf("item_id=%s\nchange_key=%s\n",
           item.item.item_id ? item.item.item_id : "NULL",
           item.item.change_key ? item.item.change_key : "NULL");

    printf("tentatively accept\n");
    //tentatively accept
    ret = ews_calendar_tentatively_accept_event(session,
                                                &item,
                                                &err_msg);
	ASSERT_TRUE(
	    IsTrue(ret == EWS_SUCCESS,
               err_msg ? err_msg : ""));

    if (err_msg) free(err_msg);
    printf("item_id=%s\nchange_key=%s\n",
           item.item.item_id ? item.item.item_id : "NULL",
           item.item.change_key ? item.item.change_key : "NULL");

    printf("accept\n");
    //accept
    ret = ews_calendar_accept_event(session,
                                    &item,
                                    &err_msg);
	ASSERT_TRUE(
	    IsTrue(ret == EWS_SUCCESS,
               err_msg ? err_msg : ""));

    if (err_msg) free(err_msg);
    printf("item_id=%s\nchange_key=%s\n",
           item.item.item_id ? item.item.item_id : "NULL",
           item.item.change_key ? item.item.change_key : "NULL");

    printf("decline\n");
    
    //decline
    ret = ews_calendar_decline_event(session,
                                     &item,
                                     &err_msg);
	ASSERT_TRUE(
	    IsTrue(ret == EWS_SUCCESS,
               err_msg ? err_msg : ""));

    if (err_msg) free(err_msg);
    printf("item_id=%s\nchange_key=%s\n",
           item.item.item_id ? item.item.item_id : "NULL",
           item.item.change_key ? item.item.change_key : "NULL");
    
    ret = ews_delete_item(session,
                          item.item.item_id,
                          item.item.change_key,
                          0,
                          &err_msg);
	ASSERT_TRUE(
	    IsTrue(ret == EWS_SUCCESS,
               err_msg ? err_msg : ""));
    

    if (err_msg) free(err_msg);

    if (item.item.item_id) free(item.item.item_id);
    if (item.item.change_key) free(item.item.change_key);
}

TEST_F(TestLibEWS, SendItem2) {
	std::auto_ptr<ews::CEWSItemOperation> pOperation(
        TestLibEWS::m_pSession->CreateItemOperation());

	ews::CEWSError error;

	std::auto_ptr<ews::CEWSMessageItem> pItem(
        ews::CEWSMessageItem::CreateInstance());

	ews::CEWSRecipientList to;
	ews::CEWSRecipientList cc;
	ews::CEWSRecipientList bcc;

	std::auto_ptr<ews::CEWSRecipient> pTo(ews::CEWSRecipient::CreateInstance());
	pTo->SetEmailAddress("stone_si@symantec.com");
	to.push_back(pTo.release());

	std::auto_ptr<ews::CEWSRecipient> pCc(ews::CEWSRecipient::CreateInstance());
	pCc->SetEmailAddress("jingnan.si@gmail.com");
	cc.push_back(pCc.release());

	std::auto_ptr<ews::CEWSRecipient> pBcc(
        ews::CEWSRecipient::CreateInstance());
	pBcc->SetEmailAddress("stone_well@hotmail.com");
	bcc.push_back(pBcc.release());

	pItem->SetBody("Test Send with libews");
	pItem->SetSubject("Test Send With libews");
	pItem->SetToRecipients(&to);
	pItem->SetCcRecipients(&cc);
	pItem->SetBccRecipients(&bcc);

	ASSERT_TRUE(IsTrue(pOperation->SendItem(pItem.get(), &error),
                       error.GetErrorMessage()));
}

TEST_F(TestLibEWS, SyncTaskItems) {
	std::auto_ptr<ews::CEWSTaskOperation> pOperation(
        TestLibEWS::m_pSession->CreateTaskOperation());

	ews::CEWSError error;
	TestItemCallback callback;
    callback.MaxReturnItemCount = 1;
    callback.includeMimeContent = ews::CEWSItemOperation::AllProperties | ews::CEWSItemOperation::TaskItem;
	ASSERT_TRUE(
        IsTrue(pOperation->SyncTask(&callback, &error),
               error.GetErrorMessage()));
}

TEST_F(TestLibEWS, GetInvalidCalendarItem) {
    std::auto_ptr<ews::CEWSItemOperation> pOperation(
        TestLibEWS::m_pSession->CreateItemOperation());
    ews::CEWSError error;

    const char * itemIds[] = {
        "AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InPtAABKXoL+pZ/4SIBV9U/tkx8+AFUiSAW+AAA=",
        "AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InPtAABKXoL+pZ/4SIBV9U/tkx8+AFUiSAWIAAA=", "AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InPtAABvCFhqbzC/To+XIiYnf+arABmqdBQGAAA=",
    };
    
    for(size_t i=0;i < sizeof(itemIds) / sizeof(const char *);i++) {
        std::auto_ptr<ews::CEWSItem> pFullItem(
            pOperation->GetItem(itemIds[i],
                                EWS_GetItems_Flags_MimeContent
                                | EWS_GetItems_Flags_AllProperties,
                                &error));

        if (pFullItem.get() == NULL)
            std::cout << itemIds[i]
                      << ","
                      << error.GetErrorMessage() << std::endl;
        else {
	TestItemCallback callback;
	callback.OutputItem(pFullItem.get());
        }
        
        ASSERT_TRUE(IsTrue(pFullItem.get() != NULL, error.GetErrorMessage()));
    }
}

TEST_F(TestLibEWS, CreateTaskItem) {
    ews_task_item item;
    memset(&item, 0, sizeof(ews_task_item));

    item.item.item_type = EWS_Item_Task;
    item.item.subject = "test with timezone utc aaa";
    item.item.body = "asldjf";
    item.start_date = time(NULL) - 86400 * 20;
    item.due_date = time(NULL) + 86444;
    item.percent_complete = 50.0;
    //item.is_complete = true;
    //item.status = EWS__TaskStatusType__Completed;

    ews_session_set_timezone_id(
                (ews_session*)TestLibEWS::m_pSession,
                "Pacific Standard Time");

    char * errmsg = NULL;
    int ret = ews_create_item_by_distinguished_id_name(
        (ews_session*)TestLibEWS::m_pSession,
        EWSDistinguishedFolderIdName_tasks,
        &item.item,
        &errmsg);

    ASSERT_TRUE(IsTrue(ret == EWS_SUCCESS, errmsg ? errmsg : ""));

    if (errmsg) free(errmsg);

    errmsg = NULL;
    
    printf("%s\n", item.item.item_id);

    item.status = EWS__TaskStatusType__Completed;
    item.complete_date = time(NULL);
    item.percent_complete = 100.0;

    ret = ews_calendar_update_task(
        (ews_session*)TestLibEWS::m_pSession,
        &item,
        0
        | EWS_UpdateCalendar_Flags_Status
        | EWS_UpdateCalendar_Flags_End
        | EWS_UpdateCalendar_Flags_Percent,
        &errmsg);
    ASSERT_TRUE(IsTrue(ret == EWS_SUCCESS, errmsg ? errmsg : ""));
}

    void out(ews_task_item * item) {
        out(&item->item);
        
SAFE_OUTPUT(item, actual_work);
SAFE_OUTPUT(item, assigned_time);
SAFE_OUTPUT_STR(item, billing_information);
SAFE_OUTPUT(item, change_count);
SAFE_OUTPUT(item, companies_count);
SAFE_OUTPUT(item, companies);
SAFE_OUTPUT(item, complete_date);
SAFE_OUTPUT(item, contacts_count);
SAFE_OUTPUT(item, contacts);
SAFE_OUTPUT(item, delegation_state);
SAFE_OUTPUT_STR(item, delegator);
SAFE_OUTPUT(item, due_date);
SAFE_OUTPUT(item, is_assignment_editable);
SAFE_OUTPUT(item, is_complete);
SAFE_OUTPUT(item, is_recurring);
SAFE_OUTPUT(item, is_team_task);
SAFE_OUTPUT_STR(item, mileage);
SAFE_OUTPUT_STR(item, owner);
SAFE_OUTPUT(item, percent_complete);
SAFE_OUTPUT(item, recurrence);
SAFE_OUTPUT(item, start_date);
SAFE_OUTPUT(item, status);
SAFE_OUTPUT_STR(item, status_description);
SAFE_OUTPUT(item, total_work);
    }

TEST_F(TestLibEWS, GetTaskItem) {
    const char * itemIds[] = {
        "AAMkADdjMTUxOWMzLWJiN2UtNGEyNC05NTIxLWI3NDhkMGM1NzgzNgBGAAAAAADjGp0ebpsORqaoBzOZqJZeBwArZr3QPuquTIplat84SmYEAEt0InPyAABKXoL+pZ/4SIBV9U/tkx8+AFUif31KAAA=",
    };
    
    for(size_t i=0;i < sizeof(itemIds) / sizeof(const char *);i++) {
        char * err_msg;
        ews_item * item = NULL;
        
        int ret = ews_get_item((ews_session*)TestLibEWS::m_pSession,
                               itemIds[i],
                                EWS_GetItems_Flags_AllProperties,
                               &item,
                               &err_msg);
                               
        if (!item)
            std::cout << itemIds[i]
                      << ","
                      << err_msg << std::endl;
        else {
            out((ews_task_item*)item);
        }
        
        ASSERT_TRUE(IsTrue(item != NULL, err_msg));
        if (item)
            ews_free_item(item);
    }
}

TEST_F(TestLibEWS, CreateTaskItem2) {
    ews_task_item item;
    memset(&item, 0, sizeof(ews_task_item));

    item.item.item_type = EWS_Item_Task;
    item.item.subject = "test with recurrence";
    item.item.body = "asldjf";
    item.start_date = time(NULL);
    item.due_date = time(NULL) + 86444;
    item.percent_complete = 50.0;
    //item.is_complete = true;
    //item.status = EWS__TaskStatusType__Completed;

    ews_recurrence recurrence;
    memset(&recurrence, 0, sizeof(ews_recurrence));

    recurrence.range.start_date = "2015-10-16";
    recurrence.range.range_type = EWSRecurrenceRangeType_NoEnd;
    
    recurrence.pattern.pattern_type = EWS_Recurrence_RelativeYearly;
    recurrence.pattern.month = EWS_MonthNames_October;
    recurrence.pattern.day_of_month = 16;

    item.recurrence = &recurrence;

    ews_session_set_timezone_id(
                (ews_session*)TestLibEWS::m_pSession,
                "Pacific Standard Time");

    char * errmsg = NULL;
    int ret = ews_create_item_by_distinguished_id_name(
        (ews_session*)TestLibEWS::m_pSession,
        EWSDistinguishedFolderIdName_tasks,
        &item.item,
        &errmsg);

    ASSERT_TRUE(IsTrue(ret == EWS_SUCCESS, errmsg ? errmsg : ""));

    if (errmsg) free(errmsg);

    errmsg = NULL;
    
    printf("%s\n", item.item.item_id);

    item.status = EWS__TaskStatusType__Completed;
    item.complete_date = time(NULL);
    item.percent_complete = 100.0;

    ret = ews_calendar_update_task(
        (ews_session*)TestLibEWS::m_pSession,
        &item,
        0
        | EWS_UpdateCalendar_Flags_Status
        | EWS_UpdateCalendar_Flags_End
        | EWS_UpdateCalendar_Flags_Percent,
        &errmsg);
    ASSERT_TRUE(IsTrue(ret == EWS_SUCCESS, errmsg ? errmsg : ""));
}
