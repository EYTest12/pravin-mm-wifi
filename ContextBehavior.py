"""ContextBehavior.py

Contains:
- ContextBehavior (class) - manages contextual execution and resources
- CustomExceptionHandler (decorator) - centralized exception handling
- FileValidationLibrary (module-level functions) - file checks and validators
- LoggingWrapper (class) - thin wrapper around Python's logging module

This is example, realistic code intended for demonstration and local testing.
"""

import logging
import os
import re
from functools import wraps
from typing import Callable, Optional, Any, Dict


class LoggingWrapper:
    """Simple logging wrapper to centralize logger configuration."""

    def __init__(self, name: str = "app", level: int = logging.INFO, logfile: Optional[str] = None):
        self.logger = logging.getLogger(name)
        if not self.logger.handlers:
            handler = logging.FileHandler(logfile) if logfile else logging.StreamHandler()
            formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
            handler.setFormatter(formatter)
            self.logger.addHandler(handler)
        self.logger.setLevel(level)

    def debug(self, msg: str, *args, **kwargs):
        self.logger.debug(msg, *args, **kwargs)

    def info(self, msg: str, *args, **kwargs):
        self.logger.info(msg, *args, **kwargs)

    def warning(self, msg: str, *args, **kwargs):
        self.logger.warning(msg, *args, **kwargs)

    def error(self, msg: str, *args, **kwargs):
        self.logger.error(msg, *args, **kwargs)


def CustomExceptionHandler(default_return: Any = None, log: Optional[LoggingWrapper] = None):
    """Decorator that centralizes exception handling and optional logging."""

    def decorator(func: Callable):
        @wraps(func)
        def wrapper(*args, **kwargs):
            try:
                return func(*args, **kwargs)
            except Exception as exc:
                if log:
                    log.error(f"Exception in {func.__name__}: {exc}")
                return default_return
        return wrapper

    return decorator


class ContextBehavior:
    """Context manager that manages a simple execution context."""

    def __init__(self, config: Optional[Dict[str, Any]] = None, logger: Optional[LoggingWrapper] = None):
        self.config = config or {}
        self._store: Dict[str, Any] = {}
        self.logger = logger or LoggingWrapper("ContextBehavior")

    def __enter__(self):
        self.logger.info("Entering context with config: %s", self.config)
        self._store["tmp_path"] = self.config.get("tmp_path", "/tmp/context_behavior")
        try:
            os.makedirs(self._store["tmp_path"], exist_ok=True)
        except Exception as e:
            self.logger.warning("Could not create tmp_path: %s", e)
        return self

    def __exit__(self, exc_type, exc, tb):
        if exc:
            self.logger.error("Exception inside context: %s", exc)
        tmp = self._store.get("tmp_path")
        if tmp and os.path.isdir(tmp):
            self.logger.info("Context finished, leaving tmp_path: %s", tmp)
        self.logger.info("Exiting context")
        return False

    def set(self, key: str, value: Any):
        self._store[key] = value
        self.logger.debug("Set context key %s", key)

    def get(self, key: str, default: Any = None) -> Any:
        return self._store.get(key, default)


ALLOWED_TEXT_EXTENSIONS = {".txt", ".md", ".csv"}
FILENAME_REGEX = re.compile(r"^[A-Za-z0-9_\-\.]{1,255}$")


def is_valid_filename(filename: str) -> bool:
    if not filename:
        return False
    if os.path.basename(filename) != filename:
        return False
    if not FILENAME_REGEX.match(filename):
        return False
    return True


def has_allowed_extension(filename: str, allowed: Optional[set] = None) -> bool:
    allowed = allowed or ALLOWED_TEXT_EXTENSIONS
    _, ext = os.path.splitext(filename)
    return ext.lower() in allowed


@CustomExceptionHandler(default_return=False, log=LoggingWrapper("FileValidation"))
def validate_file_on_disk(path: str, max_size_bytes: int = 10 * 1024 * 1024) -> bool:
    if not os.path.exists(path):
        return False
    if not os.path.isfile(path):
        return False
    if os.path.getsize(path) > max_size_bytes:
        return False
    if not is_valid_filename(os.path.basename(path)):
        return False
    if not has_allowed_extension(path):
        return False
    return True


if __name__ == "__main__":
    logger = LoggingWrapper("example", level=logging.DEBUG)
    with ContextBehavior({"tmp_path": "/tmp/example_ctx"}, logger=logger) as ctx:
        logger.info("Context tmp_path = %s", ctx.get("tmp_path"))
        print("Validating sample file (should be False):", validate_file_on_disk("/etc/hosts"))
