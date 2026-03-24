import type { NextFunction, Request, Response } from "express";

declare module "express-session" {
  interface SessionData {
    isAuthed?: boolean;
  }
}

export function requireAuth(req: Request, res: Response, next: NextFunction) {
  if (req.session.isAuthed) return next();
  return res.status(401).json({ ok: false, message: "Unauthorized" });
}
