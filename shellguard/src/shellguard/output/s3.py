"""
Send downloaded/uplaoded files to S3 (or compatible)
"""

from __future__ import annotations

from typing import Any

from configparser import NoOptionError

from botocore.exceptions import ClientError
from botocore.session import get_session

from twisted.internet import defer, threads
from twisted.python import log

import shellguard.core.output
from shellguard.core.config import ShellGuardConfig


class Output(shellguard.core.output.Output):
    """
    s3 output
    """

    def start(self) -> None:
        self.bucket = ShellGuardConfig.get("output_s3", "bucket")
        self.seen: set[str] = set()
        self.session = get_session()

        try:
            if ShellGuardConfig.get("output_s3", "access_key_id") and ShellGuardConfig.get(
                "output_s3", "secret_access_key"
            ):
                self.session.set_credentials(
                    ShellGuardConfig.get("output_s3", "access_key_id"),
                    ShellGuardConfig.get("output_s3", "secret_access_key"),
                )
        except NoOptionError:
            log.msg(
                "No AWS credentials found in config - using botocore global settings."
            )

        self.client = self.session.create_client(
            "s3",
            region_name=ShellGuardConfig.get("output_s3", "region"),
            endpoint_url=ShellGuardConfig.get("output_s3", "endpoint", fallback=None),
            verify=ShellGuardConfig.getboolean("output_s3", "verify", fallback=True),
        )

    def stop(self) -> None:
        pass

    def write(self, event: dict[str, Any]) -> None:
        if event["eventid"] == "shellguard.session.file_download":
            self.upload(event["shasum"], event["outfile"])

        elif event["eventid"] == "shellguard.session.file_upload":
            self.upload(event["shasum"], event["outfile"])

    @defer.inlineCallbacks
    def _object_exists_remote(self, shasum):
        try:
            yield threads.deferToThread(
                self.client.head_object,
                Bucket=self.bucket,
                Key=shasum,
            )
        except ClientError as e:
            if e.response["Error"]["Code"] == "404":
                defer.returnValue(False)
            raise

        defer.returnValue(True)

    @defer.inlineCallbacks
    def upload(self, shasum, filename):
        if shasum in self.seen:
            log.msg(f"Already uploaded file with sha {shasum} to S3")
            return

        exists = yield self._object_exists_remote(shasum)
        if exists:
            log.msg(f"Somebody else already uploaded file with sha {shasum} to S3")
            self.seen.add(shasum)
            return

        log.msg(f"Uploading file with sha {shasum} ({filename}) to S3")
        with open(filename, "rb") as fp:
            yield threads.deferToThread(
                self.client.put_object,
                Bucket=self.bucket,
                Key=shasum,
                Body=fp.read(),
                ContentType="application/octet-stream",
            )

        self.seen.add(shasum)
