
internal void
MB_Odin_ParseDeclSet(F4_Index_ParseCtx *ctx, F4_Index_Note *parent)
{
    F4_Index_Note *last_parent = F4_Index_PushParent(ctx, parent);
    for(;!ctx->done;)
    {
        Token *name = 0;
        if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, F4_Index_TokenSkipFlag_SkipWhitespace) &&
           F4_Index_RequireToken(ctx, S8Lit(":"), F4_Index_TokenSkipFlag_SkipWhitespace))
        {
            F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Decl, 0);
            
            for(;!ctx->done;)
            {
                Token *token = token_it_read(&ctx->it);
                if(token->sub_kind == TokenOdinKind_Comma ||
                   token->sub_kind == TokenOdinKind_Semicolon)
                {
                    F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
                    break;
                }
                else if(token->kind == TokenBaseKind_ScopeClose ||
                        token->sub_kind == TokenOdinKind_ParenCl)
                {
                    goto end;
                }
                F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
            }
            
        }
        else
        {
            break;
        }
    }
    
    end:;
    F4_Index_PopParent(ctx, last_parent);
}

internal void
MB_Odin_ParseDeclSet_Braces(F4_Index_ParseCtx *ctx, F4_Index_Note *parent)
{
    if(F4_Index_RequireToken(ctx, S8Lit("{"), F4_Index_TokenSkipFlag_SkipWhitespace))
    {
        MB_Odin_ParseDeclSet(ctx, parent);
        F4_Index_RequireToken(ctx, S8Lit("}"), F4_Index_TokenSkipFlag_SkipWhitespace);
    }
}

internal void
MB_Odin_ParseDeclSet_Parens(F4_Index_ParseCtx *ctx, F4_Index_Note *parent)
{
    if(F4_Index_RequireToken(ctx, S8Lit("("), F4_Index_TokenSkipFlag_SkipWhitespace))
    {
        MB_Odin_ParseDeclSet(ctx, parent);
        F4_Index_RequireToken(ctx, S8Lit(")"), F4_Index_TokenSkipFlag_SkipWhitespace);
    }
}

char* odin_calling_conventions[] = {
    "\"system\"",
    "\"odin\"",
    "\"contextless\"",
    "\"std\"",
    "\"stdcall\"",
    "\"cdelc\"",
    "\"fastcall\"",
    "\"fast\"",
    "\"none\"",
};

internal b32 
MB_Odin_Skip_Calling_Conventions(F4_Index_ParseCtx *ctx, F4_Index_TokenSkipFlags flags)
{
    b32 result = false;
    Token* name = 0;
    while (F4_Index_RequireTokenSubKind(ctx, TokenOdinKind_LiteralString, &name, flags))
    {
        result = true;

        /*for (u32 i = 0; i < ArrayCount(odin_calling_conventions); ++i)
        {
            String_Const_u8 string = SCu8(odin_calling_conventions[i],
                                          CalculateCStringLength(odin_calling_conventions[i]));
            if (F4_Index_PeekToken(ctx, string))
            {
                result = true;
            }
        }*/
    }
    return result;
}


internal void 
MB_Odin_Skip_PP_Stuff(F4_Index_ParseCtx *ctx, F4_Index_TokenSkipFlags flags)
{
    for (;;) 
    {
        Token *token = token_it_read(&ctx->it);
        String_Const_u8 token_string =
            string_substring(ctx->string, Ii64(token->pos, token->pos+token->size));
        
        if(token_string.str[0] == '#')
        {
            F4_Index_ParseCtx_Inc(ctx, flags);
        }
        else {
            break;
        }
    }
}

internal F4_LANGUAGE_INDEXFILE(MB_Odin_IndexFile)
{
    int scope_nest = 0;
    for(;!ctx->done;)
    {
        Token *name = 0;
        F4_Index_TokenSkipFlags flags = F4_Index_TokenSkipFlag_SkipWhitespace;
        
        // NOTE(rjf): Handle nests.
        {
            Token *token = token_it_read(&ctx->it);
            if(token)
            {
                if(token->kind == TokenBaseKind_ScopeOpen)
                {
                    scope_nest += 1;
                }
                else if(token->kind == TokenBaseKind_ScopeClose)
                {
                    scope_nest -= 1;
                }
                if(scope_nest < 0)
                {
                    scope_nest = 0;
                }
            }
        }
        
        if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, flags))
        {
            // NOTE(rjf): Definitions
            if(F4_Index_RequireToken(ctx, S8Lit("::"), flags))
            {
                MB_Odin_Skip_PP_Stuff(ctx, flags);
                // NOTE(rjf): Procedures
                if(F4_Index_RequireToken(ctx, S8Lit("proc"), flags) &&
                   (F4_Index_PeekToken(ctx, S8Lit("(")) ||
                    (MB_Odin_Skip_Calling_Conventions(ctx, flags) &&
                     F4_Index_PeekToken(ctx, S8Lit("(")))))
                {
                    F4_Index_Note *parent = F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Function, 0);
                    MB_Odin_ParseDeclSet_Parens(ctx, parent);
                }
                // NOTE(rjf): Structs
                else if(F4_Index_RequireToken(ctx, S8Lit("struct"), flags))
                {
                    F4_Index_Note *parent = F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, F4_Index_NoteFlag_ProductType);
                    MB_Odin_ParseDeclSet_Braces(ctx, parent);
                }
                // NOTE(rjf): Unions
                else if(F4_Index_RequireToken(ctx, S8Lit("union"), flags))
                {
                    F4_Index_Note *parent = F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, F4_Index_NoteFlag_SumType);
                    MB_Odin_ParseDeclSet_Braces(ctx, parent);
                }
                // NOTE(rjf): Enums
                else if(F4_Index_RequireToken(ctx, S8Lit("enum"), flags))
                {
                    F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, 0);
                }
                // NOTE(rjf): Constants
                else if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, 0, flags) ||
                        F4_Index_RequireTokenKind(ctx, TokenBaseKind_LiteralInteger, 0, flags) ||
                        F4_Index_RequireTokenKind(ctx, TokenBaseKind_LiteralFloat, 0, flags) ||
                        F4_Index_RequireTokenKind(ctx, TokenBaseKind_LiteralString, 0, flags))
                {
                    F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Constant, 0);
                }
            }
            /*
                                    else if (F4_Index_RequireToken(ctx, S8Lit(":"), flags))
                                    {
                                        if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, flags))
                                        {
                                            F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, 0);
                                        }
                        }
                        */
        }
        /*
                if(F4_Index_RequireTokenSubKind(ctx, TokenOdinKind_Colon, &name, flags))
                {
                    F4_Index_LookupNote(String_Const_u8 string);
                }
        */
        //~ NOTE(rjf): Comment Tags 
        else if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Comment, &name, F4_Index_TokenSkipFlag_SkipWhitespace))
        {
            F4_Index_ParseComment(ctx, name);
        }
        else
        {
            F4_Index_ParseCtx_Inc(ctx, flags);
        }
    }
}

internal Token *
MB_Odin_FindDecl(Application_Links *app, Buffer_ID buffer, i64 pos, Token *decl_name)
{
    Token *result = 0;
    Scratch_Block scratch(app);
    
    int scope_nest = 0;
    String_Const_u8 decl_name_str = push_buffer_range(app, scratch, buffer, Ii64(decl_name));
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
    for(;;)
    {
        Token *token = token_it_read(&it);
        if(token)
        {
            if(scope_nest == 0 &&
               token->sub_kind == TokenOdinKind_Colon &&
               token_it_dec_non_whitespace(&it))
            {
                Token *name_candidate = token_it_read(&it);
                if(name_candidate && name_candidate->kind == TokenBaseKind_Identifier)
                {
                    String_Const_u8 name_candidate_string = push_buffer_range(app, scratch, buffer, Ii64(name_candidate));
                    if(string_match(name_candidate_string, decl_name_str))
                    {
                        result = name_candidate;
                        break;
                    }
                }
            }
            else if(token->sub_kind == TokenOdinKind_BraceCl)
            {
                scope_nest += 1;
            }
            else if(token->sub_kind == TokenOdinKind_BraceOp)
            {
                scope_nest -= 1;
            }
        }
        else { break; }
        if(!token_it_dec_non_whitespace(&it))
        {
            break;
        }
    }
    return result;
}

internal F4_LANGUAGE_POSCONTEXT(MB_Odin_PosContext)
{
    int count = 0;
    F4_Language_PosContextData *first = 0;
    F4_Language_PosContextData *last = 0;
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    
    // NOTE(rjf): Search for left parentheses (function call or macro invocation).
    {
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        
        int paren_nest = 0;
        int arg_idx = 0;
        for(int i = 0; count < 4; i += 1)
        {
            Token *token = token_it_read(&it);
            if(token)
            {
                if(paren_nest == 0 &&
                   token->sub_kind == TokenOdinKind_ParenOp &&
                   token_it_dec_non_whitespace(&it))
                {
                    Token *name = token_it_read(&it);
                    if(name && name->kind == TokenBaseKind_Identifier)
                    {
                        F4_Language_PosContext_PushData_Call(arena, &first, &last, push_buffer_range(app, arena, buffer, Ii64(name)), arg_idx);
                        count += 1;
                        arg_idx = 0;
                    }
                }
                else if(token->sub_kind == TokenOdinKind_ParenOp)
                {
                    paren_nest -= 1;
                }
                else if(token->sub_kind == TokenOdinKind_ParenCl && i > 0)
                {
                    paren_nest += 1;
                }
                else if(token->sub_kind == TokenOdinKind_Comma && i > 0 && paren_nest == 0)
                {
                    arg_idx += 1;
                }
            }
            else { break; }
            if(!token_it_dec_non_whitespace(&it))
            {
                break;
            }
        }
    }
    
    // NOTE(rjf): Search for *.* pattern (accessing a type)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        
        Token *last_query_candidate = 0;
        for(int i = 0; i < 3; i += 1)
        {
            Token *token = token_it_read(&it);
            if(token)
            {
                if(i == 0 && token->kind == TokenBaseKind_Identifier)
                {
                    last_query_candidate = token;
                }
                else if((i == 0 || i == 1) &&
                        token->sub_kind == TokenOdinKind_Dot &&
                        token_it_dec_non_whitespace(&it))
                {
                    Token *decl_name = token_it_read(&it);
                    if(decl_name && decl_name->kind == TokenBaseKind_Identifier)
                    {
                        Token *decl_start = MB_Odin_FindDecl(app, buffer, decl_name->pos, decl_name);
                        if(decl_start)
                        {
                            Token_Iterator_Array it2 = token_iterator_pos(0, &tokens, decl_start->pos);
                            b32 found_colon = 0;
                            Token *base_type = 0;
                            for(;;)
                            {
                                Token *token2 = token_it_read(&it2);
                                if(token2)
                                {
                                    if(token2->sub_kind == TokenOdinKind_Colon)
                                    {
                                        found_colon = 1;
                                    }
                                    else if(found_colon && token2->kind == TokenBaseKind_Identifier)
                                    {
                                        base_type = token2;
                                    }
                                    else if(found_colon && token2->kind == TokenBaseKind_StatementClose)
                                    {
                                        break;
                                    }
                                }
                                else { break; }
                                if(!token_it_inc_non_whitespace(&it2)) { break; }
                            }
                            
                            if(base_type != 0)
                            {
                                F4_Language_PosContext_PushData_Dot(arena, &first, &last,
                                                                    push_buffer_range(app, arena, buffer, Ii64(base_type)), last_query_candidate);
                            }
                        }
                    }
                }
            }
            else { break; }
            if(!token_it_dec_non_whitespace(&it))
            {
                break;
            }
        }
    }
    
    return first;
}

internal F4_LANGUAGE_HIGHLIGHT(MB_Odin_Highlight)
{
}