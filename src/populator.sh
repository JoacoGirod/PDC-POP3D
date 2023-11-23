#!/bin/bash

# Function to create a mailbox for a user
create_mailbox() {
    local maildir_path="$1"
    local username="$2"

    local user_path="$maildir_path/$username/Maildir"
    mkdir -p "$user_path"

    for folder in "new" "tmp" "cur"; do
        mkdir -p "$user_path/$folder"
    done

    # Sample email for the user
    local email_content="Return-Path: <$username@foo.pdc>\r\n
                        X-Original-To: $username@foo.pdc\r\n
                        Delivered-To: $username@foo.pdc\r\n
                        Received: from $username (localhost [127.0.0.1])\r\n
                            by $username.foo.pdc (Postfix) with ESMTP id 123456789\r\n
                            for <$username@foo.pdc>; Thu, 31 Aug 2023 12:00:00 -0300 (-03)\r\n
                        Subject: Sample Email\r\n
                        From: $username@foo.pdc\r\n
                        Date: Thu, 31 Aug 2023 12:00:00 -0300 (-03)\r\n
                        \r\n
                        This is a sample email for $username.\r\n
                        "

    # Populate cur directory with the sample email
    echo -e "$email_content" >"$user_path/cur/email_$username"

    echo "Mailbox for $username created successfully."
}

# Function to create Maildir structure and populate with sample emails
create_maildir_structure() {
    local maildir_path="/tmp"
    mkdir -p "$maildir_path"

    local user_path="$maildir_path/testuser/Maildir"
    mkdir -p "$user_path"

    for folder in "new" "tmp" "cur"; do
        mkdir -p "$user_path/$folder"
    done

    # Create mailbox for the second user (testuser2)
    create_mailbox "$maildir_path" "testuser2"

    # Create mailbox for the third user (testuser3)
    create_mailbox "$maildir_path" "testuser3"

    # Sample emails
    local email_content1="Return-Path: <joaquin@foo.pdc>\r\n
                            X-Original-To: joaquin@foo.pdc\r\n
                            Delivered-To: joaquin@foo.pdc\r\n
                            Received: from jgirod (localhost [127.0.0.1])\r\n
                                by jgirod.foo.pdc (Postfix) with ESMTP id 1B6D917F014\r\n
                                for <joaquin@foo.pdc>; Thu, 31 Aug 2023 11:19:47 -0300 (-03)\r\n
                            subject: felicidad\r\n
                            Message-Id: <20230831141950.1B6D917F014@jgirod.foo.pdc>\r\n
                            Date: Thu, 31 Aug 2023 11:19:47 -0300 (-03)\r\n
                            From: joaquin@foo.pdc\r\n
                            from jose@gmail.com\r\n
                            dfsasd\r\n
                            .\r\n
                            Hola parte 2....\r\n
                            \r\n.\r\n
                            .Hola parte 3.\r\n
                            .\r\n
                            HOAL COMO ESTAS TODO BIEN?\r\n"

    local email_content2="Return-Path: <joaquin@foo.pdc>\n
                        X-Original-To: joaquin@foo.pdc\n
                        Delivered-To: joaquin@foo.pdc\n
                        Received: from jgirod (unknown [192.168.1.6])\n
                            by joaquin.foo.pdc (Postfix) with ESMTP id 4075017ED0A\n
                            for <joaquin@foo.pdc>; Thu, 31 Aug 2023 11:33:33 -0300 (-03)\n
                        Subject: What is love \n
                        From: <flopezmenardi@itba.edu.ar>\n
                        To: <cijjas@itba.edu.ar>\n
                        Message-Id: <950124.162336@example.com>\n
                        Date: Thu, 31 Aug 2023 11:33:33 -0300 (-03)\n

                        This is a great message
                        "
    local email_content3="A : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        B : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        C : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        D : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        E : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        F : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        G : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        H : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        I : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        J : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        K : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        L : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        M : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        N : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        O : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        P : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        Q : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        R : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        S : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        T : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        U : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        V : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        W : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        Y : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        Z : XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n
                        "
    # Populate cur directory with sample emails
    echo -e "$email_content1" >"$user_path/cur/email1"
    echo -e "$email_content2" >"$user_path/cur/email2"

    # Populate new directory with sample emails
    echo -e "$email_content3" >"$user_path/new/email3"
    echo -e "$email_content2" >"$user_path/new/email4"

    echo "Maildir structure created successfully."
}

# Create Maildir structure and populate with sample emails
create_maildir_structure

# Create a 100MB file using dd command
dd if=/dev/urandom of=/tmp/testuser/Maildir/cur/large_email bs=4096 count=25600

echo "100MB file created successfully."
