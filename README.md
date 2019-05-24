# SafeDrive

SafeDrive is a file backup tool that encrypts and keeps files safe on
remote servers. It can be used for safely backing up sensitive files to
Google Drive, Dropbox and similar services.

### Specifications

The SafeDrive database file structure and protocol is open source and
public domain. Please read the SPECS file for more information.

### Requirements

Make sure you have the following libraries installed before attempting
to compile these sources. If you do not have them all installed SafeDrive
will not compile. You also need the gcc compiler (apt-get install gcc).

##### Required Libraries:
```
GPGME (apt-get install libgpgme-dev)
cURL (apt-get install libcurl3)
OpenSSL (apt-get install libssl-dev)
```
