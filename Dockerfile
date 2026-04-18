FROM fedora:43
ARG XWIN_VERSION=0.7.0
ENV XWIN_VERSION=${XWIN_VERSION}

WORKDIR /app 

RUN dnf update -y 
RUN dnf install -y clang clang-libs clang-resource-filesystem llvm llvm-libs llvm-devel wget curl make python3

# Fetch xwin
RUN wget "https://github.com/Jake-Shadle/xwin/releases/download/${XWIN_VERSION}/xwin-${XWIN_VERSION}-x86_64-unknown-linux-musl.tar.gz" -O xwin.tar.gz 
RUN tar -xvzf xwin.tar.gz 
RUN chmod +x xwin-${XWIN_VERSION}-x86_64-unknown-linux-musl/xwin
RUN xwin-${XWIN_VERSION}-x86_64-unknown-linux-musl/xwin --arch x86,x86_64 --accept-license splat --output /opt/winsdk --preserve-ms-arch-notation
RUN rm -rf *

COPY boflint.py /app/boflint.py
COPY Makefile /app/Makefile 
CMD ["make"]