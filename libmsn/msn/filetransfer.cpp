/*
 * filetransfer.cpp
 * libmsn
 *
 * Created by Mark Rowe on Wed Mar 17 2004.
 * Copyright (c) 2004 Mark Rowe. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <msn/filetransfer.h>
#include <msn/message.h>
#include <msn/errorcodes.h>
#include <msn/externals.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#include <io.h>
#endif

#include <cassert>
#include <cerrno>

namespace MSN 
{
    const unsigned int MAX_FTP_BLOCK_SIZE = 20000;
    
    void FileTransferInvitation::invitationWasAccepted(const std::string & body)
    {
        if (this->invitationWasSent())
        {
            this->sendFile(body);
        }
        else
        {
            this->receiveFile(body);
        }
    }
    
    void FileTransferInvitation::invitationWasCanceled(const std::string & body)
    {
        ext::fileTransferFailed(this, 0, "Cancelled by remote user");
        if (this->invitationWasSent())
        {
            this->switchboardConnection->invitationsSent.remove(this);
        }
        else 
        {
            this->switchboardConnection->invitationsReceived.remove(this);
        }
        this->switchboardConnection->removeFileTransferConnection(this);
    }
    
    void FileTransferInvitation::sendFile(const std::string & msg_body)
    {
        int port = 6891;
        char tmp[64];
        sprintf(tmp, "%d", rand());
        FileTransferConnection::AuthData auth = FileTransferConnection::AuthData(this->switchboardConnection->auth.username, 
                                                                                 std::string(tmp), FileTransferConnection::MSNFTP_SEND, this);
        FileTransferConnection * conn = new FileTransferConnection(auth);
        
        ext::fileTransferProgress(this, "Sending IP address", 0, 0);
        
        while((conn->sock = ext::listenOnPort(port)) < 0)
        {
            port++;
            if (port > 6911)
            {
                ext::fileTransferFailed(this, errno, strerror(errno));
                this->switchboardConnection->invitationsSent.remove(this);
                conn->disconnect();
                return;
            }
        }
        
        ext::registerSocket(conn->sock, 1, 0);
        
        this->switchboardConnection->addFileTransferConnection(conn);
        
        std::ostringstream buf_;
        buf_ << "Invitation-Command: ACCEPT\r\n";
        buf_ << "Invitation-Cookie: " << this->cookie << "\r\n";
        buf_ << "IP-Address: " << ext::getOurIP() << "\r\n";
        buf_ << "Port: " << port << "\r\n";
        buf_ << "AuthCookie: " << conn->auth.cookie << "\r\n";
        buf_ << "Launch-Application: FALSE\r\n";
        buf_ << "Request-Data: IP-Address:\r\n";
        buf_ << "\r\n";
        
        Message * msg = new Message(buf_.str());
        msg->setHeader("Content-Type", "text/x-msmsgsinvite; charset=UTF-8");
        this->switchboardConnection->sendMessage(msg);
        delete msg;
    }
    
    void FileTransferInvitation::receiveFile(const std::string & msg_body)
    {
        Message::Headers headers = Message::Headers(msg_body);
        std::string cookie = headers["AuthCookie"];
        std::string remote = headers["IP-Address"];
        std::string port_c = headers["Port"];
        int port;
        
        if (cookie.empty() || remote.empty() || port_c.empty())
        {
            ext::fileTransferFailed(this, 0, "Missing parameters");
            this->switchboardConnection->invitationsReceived.remove(this);
            return;
        }
        
        port = decimalFromString(port_c);
        
        FileTransferConnection::AuthData auth = FileTransferConnection::AuthData(this->switchboardConnection->auth.username,
                                                                                 cookie, FileTransferConnection::MSNFTP_RECV, this);
        FileTransferConnection * conn = new FileTransferConnection(auth);
        
        std::ostringstream buf_;
        buf_ << "Connecting to " << remote << ":" << port << "\n";
        ext::fileTransferProgress(this, buf_.str(), 0, 0);
        
        conn->sock = ext::connectToServer(remote, port, &conn->connected);
        
        if (conn->sock < 0)
        {
            ext::fileTransferFailed(this, errno, strerror(errno));
            this->switchboardConnection->invitationsReceived.remove(this);
            return;
        }

        if (! conn->isConnected())
            ext::registerSocket(conn->sock, 0, 1);
        else
            ext::registerSocket(conn->sock, 1, 0);
        
        ext::fileTransferProgress(this, "Connected", 0, 0);
        this->switchboardConnection->addFileTransferConnection(conn);
        
        conn->write("VER MSNFTP\r\n");
    }
    
    void FileTransferConnection::disconnect()
    {
        this->auth.inv->switchboardConnection->removeFileTransferConnection(this);
        delete this->auth.inv;
        this->auth.inv = NULL;
        Connection::disconnect();
    }
    
    FileTransferConnection::~FileTransferConnection()
    {
        this->disconnect();
    }
    
    void FileTransferConnection::socketConnectionCompleted()
    {
        Connection::socketConnectionCompleted();
        ext::unregisterSocket(this->sock);
        ext::registerSocket(this->sock, 1, 0);
    }    
    
    void FileTransferConnection::socketIsWritable()
    {
        if (this->auth.direction == MSNFTP_SEND)
            this->handleSend();
    }
    
    void FileTransferConnection::dataArrivedOnSocket()
    {
        if (this->auth.direction == MSNFTP_SEND && ! this->auth.connected)
            this->handleSend();
        else
            Connection::dataArrivedOnSocket();
    }
    
    void FileTransferConnection::handleIncomingData()
    {
        if (this->auth.direction == MSNFTP_RECV)
            this->handleReceive();
        else 
            this->handleSend();
    }
    
    void FileTransferConnection::handleSend()
    {
        if (! this->auth.connected) // we have not accept()ed yet, but the read/writability means there's one waiting
        {
            this->handleSend_WaitingForConnection();
        } 
        else if (this->auth.fd == NULL)
        {
            this->handleSend_Negotiating();
        }
        else if (this->auth.inv->fileSize != this->auth.bytes_done)
        {
            this->handleSend_Transferring();
        }
        else
        {
            this->handleSend_Bye();
        }
    }
    
    void FileTransferConnection::handleSend_WaitingForConnection()
    {
        int s;
        
        if ((s = accept(this->sock, NULL, NULL)) < 0)
        {
            perror("Could not accept()\n");
            ext::fileTransferFailed(this->auth.inv, errno, strerror(errno));
            this->auth.inv->switchboardConnection->invitationsSent.remove(this->auth.inv);
            return;
        }
        
        ext::unregisterSocket(this->sock);
        close(this->sock);
        
        this->sock = s;
        ext::registerSocket(this->sock, 1, 0);
        
        ext::fileTransferProgress(this->auth.inv, "Connected", 0, 0);
        
        this->auth.connected = 1;
        this->connected = true;        
    }
    
    void FileTransferConnection::handleSend_Negotiating()
    {
        if (! this->isWholeLineAvailable())
            return;
        
        std::vector<std::string> args = this->getLine();
        this->readBuffer = this->readBuffer.substr(this->readBuffer.find("\r\n") + 2);
                
        if (args[0] == "VER")
        {
            this->write("VER MSNFTP\r\n");
            ext::fileTransferProgress(this->auth.inv, "Negotiating", 0, 0);
        }
        else if (args[0] == "USR")
        {
            if (args[2] != this->auth.cookie)  // if they DIFFER
            {
                ext::fileTransferFailed(this->auth.inv, errno, strerror(errno));
                this->auth.inv->switchboardConnection->invitationsSent.remove(this->auth.inv);
                return;
            }
            std::ostringstream buf_;
            buf_ << "FIL " << this->auth.inv->fileSize << "\r\n";
            this->write(buf_);
        }
        else if (args[0] == "TFR")
        {
            // you asked for it, go to data-dump mode
            this->auth.fd = fopen(this->auth.inv->fileName.c_str(), "r");
            if (this->auth.fd == NULL)
            {
                perror("fopen() failed");
                ext::fileTransferFailed(this->auth.inv, errno, "Could not open file for reading");
                this->auth.inv->switchboardConnection->invitationsSent.remove(this->auth.inv);
                return;
            }
            
            // OK, now we lose control, but the next round of the polling loop will
            // say that the socket is writable, and then the fun starts...
            ext::fileTransferProgress(this->auth.inv, "Sending data", 0, 0);
            ext::unregisterSocket(this->sock);
            ext::registerSocket(this->sock, 0, 1);                        
        }
    }
    
    void FileTransferConnection::handleSend_Transferring()
    {
        // just pumping data now
        
        fd_set writefd;
        FD_ZERO(&writefd);
        FD_SET(this->sock, &writefd);
        struct timeval tout={0, 0};
        unsigned char *readBuffer = (unsigned char *) calloc(MAX_FTP_BLOCK_SIZE, sizeof(unsigned char));
        
        if (select(this->sock + 1, NULL, &writefd, NULL, &tout) == 1)
        {
            int bytesWritten = 0;
            unsigned int blockLength;
            unsigned char blockHeader[3];
            unsigned int bytesRemaining = this->auth.inv->fileSize - this->auth.bytes_done;
            blockLength = bytesRemaining > MAX_FTP_BLOCK_SIZE / 4 ? MAX_FTP_BLOCK_SIZE / 4 : bytesRemaining;
            if (blockLength == 0)
                blockHeader[0] = 1;
            else
                blockHeader[0] = 0;
            blockHeader[1] = (blockLength >> 0) & 0xff;
            blockHeader[2] = (blockLength >> 8) & 0xff;
            
            if (this->write(std::string((char *) &blockHeader[bytesWritten], 3 - bytesWritten), false) < 0)
            {
                ext::fileTransferFailed(this->auth.inv, errno, strerror(errno));
                goto cleanup;
            }
            
            if (fread(readBuffer, sizeof(unsigned char), blockLength, this->auth.fd) < 0)
            {
                ext::fileTransferFailed(this->auth.inv, errno, strerror(errno));
                goto cleanup;
            }
            
            if (this->write(std::string((char *) readBuffer, blockLength), false) < 0)
            {
                ext::fileTransferFailed(this->auth.inv, errno, strerror(errno));
                goto cleanup;
            }
            this->auth.bytes_done += blockLength;
        }
        free(readBuffer);
        ext::fileTransferProgress(this->auth.inv, "Sending file", this->auth.bytes_done, this->auth.inv->fileSize);
        return;
cleanup:
            ;
        this->auth.inv->switchboardConnection->invitationsSent.remove(this->auth.inv);
        if (readBuffer)
            free(readBuffer);
    }
    
    void FileTransferConnection::handleSend_Bye()
    {
        if (! this->isWholeLineAvailable())
            return;
        
        std::vector<std::string> args;
        ext::fileTransferSucceeded(this->auth.inv);
        
        args = this->getLine();
        this->readBuffer = this->readBuffer.substr(this->readBuffer.find("\r\n") + 2);

        if (args.size() > 0)
        {
            printf("%s", args[0].c_str());
            if (args.size() > 1)
                printf(" %s", args[1].c_str());
            printf("\n");
        }
        
        this->auth.inv->switchboardConnection->invitationsSent.remove(this->auth.inv);        
    }
    
    
    void FileTransferConnection::handleReceive()
    {
        if (this->auth.fd == NULL)
            this->handleReceive_Negotiating();
        else
            this->handleReceive_Transferring();
    }
    
    void FileTransferConnection::handleReceive_Negotiating()
    {
        if (! this->isWholeLineAvailable())
            return;
                
        std::vector<std::string> args = this->getLine();
        this->readBuffer = this->readBuffer.substr(this->readBuffer.find("\r\n") + 2);        
        
        if (args[0] == "VER")
        {
            std::ostringstream buf_;
            buf_ << "USR " << this->auth.username << " " << this->auth.cookie << "\r\n";
            this->write(buf_);
            ext::fileTransferProgress(this->auth.inv, "Negotiating", 0, 0);
            return;
        } 
        else if (args[0] == "FIL")
        {
            this->auth.fd = fopen(this->auth.inv->fileName.c_str(), "w");
            if (this->auth.fd == NULL)
                goto error;
            
            this->write("TFR\r\n");
        }
        return;
error:
            ;
        ext::fileTransferFailed(this->auth.inv, errno, strerror(errno));
        this->switchboardConnection()->invitationsReceived.remove(this->auth.inv);
    }
    
    void FileTransferConnection::handleReceive_Transferring()
    {
        // Each block has a 3-byte header
        // 0: either 1 or 0.  0 means data block, 1 means transfer completed.
        // 1: Low byte of block length
        // 2: High byte of block length
        
        unsigned int blockLength;
        std::string blockHeader;
        
        while (1)
        {
            if (this->readBuffer.size() < 3)
                return;
            
            // read the three byte header
            blockHeader = this->readBuffer.substr(0, 3);
            if (blockHeader[0] == 1U)
            {
                // Transfer completed block
                if (blockHeader[1] != 0 || blockHeader[2] != 0)
                {
                    ext::fileTransferFailed(this->auth.inv, 0, "Invalid block header.\n");
                    goto cleanup;
                }
                this->write("BYE 16777989\r\n");
                ext::fileTransferSucceeded(this->auth.inv);
                
                goto cleanup;
            }
            else if (blockHeader[0] != 0U)
            {
                ext::fileTransferFailed(this->auth.inv, 0, "Invalid block header.");
                goto cleanup;
            }
            
            // Read the entire block
            blockLength = ((unsigned char) blockHeader[1]) | ((unsigned char) blockHeader[2]) << 8;
            if (blockLength > MAX_FTP_BLOCK_SIZE)
            {
                ext::fileTransferFailed(this->auth.inv, 0, "Block size greater than largest expected block size.");
                goto cleanup;
            }
            
            if (this->readBuffer.size() < 3 + blockLength)
                return;
            
            std::string block = this->readBuffer.substr(3, blockLength);
            this->readBuffer = this->readBuffer.substr(3 + blockLength);
            
            fwrite(block.c_str(), sizeof(unsigned char), block.size(), this->auth.fd);
            this->auth.bytes_done += blockLength;
            
            if (this->auth.bytes_done == this->auth.inv->fileSize)
            {
                // It appears that sometimes the 1,0,0 block header
                // that indicates success isnt sent...
                
                this->write("BYE 16777989\r\n");
                ext::fileTransferSucceeded(this->auth.inv);
                goto cleanup;
            }
            ext::fileTransferProgress(this->auth.inv, "Receiving file", this->auth.bytes_done, this->auth.inv->fileSize);
        }
        return;
cleanup:
            ;
        this->auth.inv->switchboardConnection->invitationsReceived.remove(this->auth.inv);
    }
    
    void FileTransferInvitation::rejectTransfer()
    {
        std::ostringstream buf_;
        buf_ << "Invitation-Command: CANCEL\r\n";
        buf_ << "Invitation-Cookie: " << this->cookie << "\r\n";
        buf_ << "Cancel-Code: REJECT\r\n";
        
        Message *msg = new Message(buf_.str());
        msg->setHeader("Content-Type", "text/x-msmsgsinvite; charset=UTF-8");
        this->switchboardConnection->sendMessage(msg);
        delete msg;
        
        this->switchboardConnection->invitationsReceived.remove(this);
    }
    
    void FileTransferInvitation::acceptTransfer(const std::string & dest)
    {
        std::ostringstream buf_;
        buf_ << "Invitation-Command: ACCEPT\r\n";
        buf_ << "Invitation-Cookie: " << (this->cookie.empty() ? "" : this->cookie) << "\r\n";
        buf_ << "Launch-Application: FALSE\r\n";
        buf_ << "Request-Data: IP-Address\r\n";
        buf_ << "\r\n";
        
        this->fileName = dest;
        Message *msg = new Message(buf_.str());
        msg->setHeader("Content-Type", "text/x-msmsgsinvite; charset=UTF-8");
        this->switchboardConnection->sendMessage(msg);
        delete msg;
        
    }
    
    void FileTransferInvitation::cancelTransfer()
    {
        std::ostringstream buf_;
        buf_ << "Invitation-Command: CANCEL\r\n";
        buf_ << "Invitation-Cookie: " << this->cookie << "\r\n";
        buf_ << "Cancel-Code: OUTBANDCANCEL\r\n";
        
        Message *msg = new Message(buf_.str());
        msg->setHeader("Content-Type", "text/x-msmsgsinvite; charset=UTF-8");
        this->switchboardConnection->sendMessage(msg);
        delete msg;    
        
        // one of the two below will fail, but it will do so safely and quietly
        this->switchboardConnection->invitationsReceived.remove(this);
        this->switchboardConnection->invitationsSent.remove(this);
        
        this->switchboardConnection->removeFileTransferConnection(this);
    }
}
