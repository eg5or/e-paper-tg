FROM node:20-alpine

WORKDIR /app

COPY package.json tsconfig.json vite.config.ts index.html ./
RUN npm install

COPY src ./src

ENV NODE_ENV=production
ENV BACKEND_PORT=3028

EXPOSE 3028

RUN npm run build:client

CMD ["npm", "run", "start"]
