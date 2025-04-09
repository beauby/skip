ARG COMMIT_HASH=deadbeef
FROM skiplabs/skip-load-tests-base:${COMMIT_HASH}

COPY . /server
WORKDIR /server
RUN npm install /packages/* && \
    npm install && \
    npm run build

EXPOSE 8080/tcp
EXPOSE 8081/tcp
ENTRYPOINT ["npm", "start", "--"]
